#include <linux/videodev2.h>
#include <cstdio>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <glob.h>

#include <kms++/kms++.h>
#include <kms++util/kms++util.h>

#include <stdexcept> // std::invalid_argument
#include <iostream>  // std::cerr
#include <string>

#define CAMERA_BUF_QUEUE_SIZE	5
#define MAX_CAMERA		9

using namespace std;
using namespace kms;

enum class BufferProvider {
	DRM,
	V4L2,
};

class CameraPipeline
{
public:
	CameraPipeline(int cam_fd, Card& card, Crtc* crtc, Plane* plane, uint32_t x, uint32_t y,
		       uint32_t iw, uint32_t ih, PixelFormat pixfmt,
		       BufferProvider buffer_provider, unsigned cnt_of_cams_);
	~CameraPipeline();

	CameraPipeline(const CameraPipeline& other) = delete;
	CameraPipeline& operator=(const CameraPipeline& other) = delete;

	void show_next_frame(AtomicReq &req);
	int fd() const { return m_fd; }
	void start_streaming();
private:
	DmabufFramebuffer* GetDmabufFrameBuffer(Card& card, uint32_t i, PixelFormat pixfmt);
	int m_fd;	/* camera file descriptor */
	Crtc* m_crtc;
	Plane* m_plane;
	BufferProvider m_buffer_provider;
	vector<Framebuffer*> m_fb;
	int m_prev_fb_index;
	uint32_t m_in_width, m_in_height; /* camera capture resolution */
	/* image properties for display */
	uint32_t m_out_width, m_out_height;
	uint32_t m_out_x, m_out_y;
	unsigned cnt_of_cams;
};

static int buffer_export(int v4lfd, enum v4l2_buf_type bt, uint32_t index, int *dmafd)
{
	struct v4l2_exportbuffer expbuf;

	memset(&expbuf, 0, sizeof(expbuf));
	expbuf.type = bt;
	expbuf.index = index;
	if (ioctl(v4lfd, VIDIOC_EXPBUF, &expbuf) == -1) {
		perror("VIDIOC_EXPBUF");
		return -1;
	}

	*dmafd = expbuf.fd;

	return 0;
}

DmabufFramebuffer* CameraPipeline::GetDmabufFrameBuffer(Card& card, uint32_t i, PixelFormat pixfmt)
{
	int r, dmafd;

	r = buffer_export(m_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, i, &dmafd);
	ASSERT(r == 0);

	const PixelFormatInfo& format_info = get_pixel_format_info(pixfmt);
	ASSERT(format_info.num_planes == 1);

	vector<int> fds { dmafd };
	vector<uint32_t> pitches { m_in_width * (format_info.planes[0].bitspp / 8) };
	vector<uint32_t> offsets { 0 };

	return new DmabufFramebuffer(card, m_in_width, m_in_height, pixfmt,
				  fds, pitches, offsets);
}

bool inline better_size(struct v4l2_frmsize_discrete* v4ldisc,
			uint32_t iw, uint32_t ih,
			uint32_t best_w, uint32_t best_h)
{
	if (v4ldisc->width <= iw && v4ldisc->height <= ih &&
	    (v4ldisc->width >= best_w || v4ldisc->height >= best_h))
		return true;

	return false;
}

CameraPipeline::CameraPipeline(int cam_fd, Card& card, Crtc *crtc, Plane* plane, uint32_t x, uint32_t y,
			       uint32_t iw, uint32_t ih, PixelFormat pixfmt,
			       BufferProvider buffer_provider, unsigned cnt_of_cams_)
	: m_fd(cam_fd), m_crtc(crtc), m_buffer_provider(buffer_provider), m_prev_fb_index(-1), cnt_of_cams(cnt_of_cams_)
{

	int r;
	uint32_t best_w = 704;
	uint32_t best_h = 280;

	struct v4l2_frmsizeenum v4lfrms = { };
	v4lfrms.pixel_format = (uint32_t)pixfmt;
	while (ioctl(m_fd, VIDIOC_ENUM_FRAMESIZES, &v4lfrms) == 0) {
		if (v4lfrms.type != V4L2_FRMSIZE_TYPE_DISCRETE) {
			v4lfrms.index++;
			continue;
		}

		if (v4lfrms.discrete.width > iw || v4lfrms.discrete.height > ih) {
			//skip
		} else if (v4lfrms.discrete.width == iw && v4lfrms.discrete.height == ih) {
			// Exact match
			best_w = v4lfrms.discrete.width;
			best_h = v4lfrms.discrete.height;
			break;
		} else if (v4lfrms.discrete.width >= best_w || v4lfrms.discrete.height >= ih) {
			best_w = v4lfrms.discrete.width;
			best_h = v4lfrms.discrete.height;
		}

		v4lfrms.index++;
	};

	m_out_width = m_in_width = best_w;
	m_out_height = m_in_height = best_h;
	/* Move it to the middle of the requested area */
	m_out_x = x ;// + iw / 2 - (m_out_width / 2)/cnt_of_cams;
	m_out_y = y + ih / 2 - (m_out_height / 2);

	printf("x, y: %ux%u\n", x, y);
	printf("iw, ih: %ux%u\n", iw, ih);
	printf("m_out_width, m_out_height: %ux%u\n", m_out_width, m_out_height);
	printf("m_out_x, m_out_y: %ux%u\n", m_out_x, m_out_y);
	printf("Capture: %ux%u\n", best_w, best_h);

	struct v4l2_format v4lfmt = { };
	v4lfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	r = ioctl(m_fd, VIDIOC_G_FMT, &v4lfmt);
	ASSERT(r == 0);

	v4lfmt.fmt.pix.pixelformat = (uint32_t)pixfmt;
	v4lfmt.fmt.pix.width = m_in_width;
	v4lfmt.fmt.pix.height = m_in_height;

	r = ioctl(m_fd, VIDIOC_S_FMT, &v4lfmt);
	ASSERT(r == 0);

	uint32_t v4l_mem;

	if (m_buffer_provider == BufferProvider::V4L2)
		v4l_mem = V4L2_MEMORY_MMAP;
	else
		v4l_mem = V4L2_MEMORY_DMABUF;

	struct v4l2_requestbuffers v4lreqbuf = { };
	v4lreqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4lreqbuf.memory = v4l_mem;
	v4lreqbuf.count = CAMERA_BUF_QUEUE_SIZE;
	r = ioctl(m_fd, VIDIOC_REQBUFS, &v4lreqbuf);
	ASSERT(r == 0);
	ASSERT(v4lreqbuf.count == CAMERA_BUF_QUEUE_SIZE);

	struct v4l2_buffer v4lbuf = { };
	v4lbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4lbuf.memory = v4l_mem;

	for (unsigned i = 0; i < CAMERA_BUF_QUEUE_SIZE; i++)
	{
		Framebuffer *fb;

		try
		{
			if (m_buffer_provider == BufferProvider::V4L2)
				// allocating special for MMAP memory
				fb = GetDmabufFrameBuffer(card, i, pixfmt);
			else
				fb = new DumbFramebuffer(card, m_in_width,
							 m_in_height, pixfmt);
		}
		catch(const std::invalid_argument &ia)
		{
			std::cerr << "Invalid argument: " << ia.what() << '\n';
		}

		v4lbuf.index = i;
		if (m_buffer_provider == BufferProvider::DRM)
			v4lbuf.m.fd = fb->prime_fd(0);
		r = ioctl(m_fd, VIDIOC_QBUF, &v4lbuf);
		ASSERT(r == 0);

		// added buffer to vector of buffers
		m_fb.push_back(fb);
	}

	m_plane = plane;

	// Do initial plane setup with first fb, so that we only need to
	// set the FB when page flipping
	AtomicReq req(card);

	Framebuffer *fb = m_fb[0];

	printf("CRTC_ID: %d\n", m_crtc->id());
	req.add(m_plane, "CRTC_ID", m_crtc->id());
	printf("FB_ID: %d\n", fb->id());
	req.add(m_plane, "FB_ID", fb->id());

	printf("CRTC_X: %d\n", m_out_x);
	req.add(m_plane, "CRTC_X", m_out_x);
	printf("CRTC_Y: %d\n", m_out_y);
	req.add(m_plane, "CRTC_Y", m_out_y);
	printf("CRTC_W: %d\n", iw); //m_out_width/cnt_of_cams);
	req.add(m_plane, "CRTC_W", iw); //m_out_width/cnt_of_cams);
	printf("CRTC_H: %d\n", m_in_height); //m_out_height/cnt_of_cams);
	req.add(m_plane, "CRTC_H", m_in_height); //m_out_height/cnt_of_cams);

	printf("SRC_X: %d\n", 0);
	req.add(m_plane, "SRC_X", 0);
	printf("SRC_Y: %d\n", 0);
	req.add(m_plane, "SRC_Y", 0);
	printf("SRC_W: %d\n", m_in_width);
	req.add(m_plane, "SRC_W", m_in_width << 16);
	printf("SRC_H: %d\n", m_in_height);
	req.add(m_plane, "SRC_H", m_in_height << 16);

	r = req.commit_sync();
	FAIL_IF(r, "initial plane setup failed");
}

CameraPipeline::~CameraPipeline()
{
	for (unsigned i = 0; i < m_fb.size(); i++)
		delete m_fb[i];

	::close(m_fd);
}

void CameraPipeline::start_streaming()
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	int r = ioctl(m_fd, VIDIOC_STREAMON, &type);
	FAIL_IF(r, "Failed to enable camera stream: %d", r);
}

void CameraPipeline::show_next_frame(AtomicReq& req)
{
	int r;
	uint32_t v4l_mem;

	if (m_buffer_provider == BufferProvider::V4L2)
		v4l_mem = V4L2_MEMORY_MMAP;
	else
		v4l_mem = V4L2_MEMORY_DMABUF;

	struct v4l2_buffer v4l2buf = { };
	v4l2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2buf.memory = v4l_mem;
	r = ioctl(m_fd, VIDIOC_DQBUF, &v4l2buf);
	if (r != 0) {
		printf("VIDIOC_DQBUF ioctl failed with %d\n", errno);
		return;
	}

	unsigned fb_index = v4l2buf.index;

	Framebuffer *fb = m_fb[fb_index];

	//req.add(m_plane, "FB_ID", fb->id());

	if (m_prev_fb_index >= 0) {
		memset(&v4l2buf, 0, sizeof(v4l2buf));
		v4l2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		v4l2buf.memory = v4l_mem;
		v4l2buf.index = m_prev_fb_index;
		if (m_buffer_provider == BufferProvider::DRM)
			v4l2buf.m.fd = m_fb[m_prev_fb_index]->prime_fd(0);
		r = ioctl(m_fd, VIDIOC_QBUF, &v4l2buf);
		ASSERT(r == 0);

	}

	m_prev_fb_index = fb_index;

	// position does't matter
	req.add(m_plane, "FB_ID", fb->id());
}

static bool is_capture_dev(int fd)
{
	struct v4l2_capability cap = { };
	int r = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	ASSERT(r == 0);
	return cap.capabilities & V4L2_CAP_VIDEO_CAPTURE;
}

std::vector<std::string> glob(const std::string& pat)
{
	glob_t glob_result;
	glob(pat.c_str(), 0, NULL, &glob_result);
	vector<string> ret;
	for(unsigned i = 0; i < glob_result.gl_pathc; ++i)
		ret.push_back(string(glob_result.gl_pathv[i]));
	globfree(&glob_result);
	return ret;
}

static const char* usage_str =
		"Usage: kmscapture [OPTIONS]\n\n"
		"Options:\n"
		"  -s, --single                Single camera mode. Open only /dev/video0\n"
		"      --buffer-type=<drm|v4l> Use DRM or V4L provided buffers. Default: DRM\n"
		"  -h, --help                  Print this help\n"
		;

int main(int argc, char** argv)
{
	BufferProvider buffer_provider = BufferProvider::V4L2;
	//BufferProvider buffer_provider = BufferProvider::DRM;
	bool single_cam = false;

	OptionSet optionset = {
		Option("s|single", [&]()
		{
			single_cam = true;
		}),
		Option("|buffer-type=", [&](string s)
		{
			if (s == "v4l")
				buffer_provider = BufferProvider::V4L2;
			else if (s == "drm")
				buffer_provider = BufferProvider::DRM;
			else
				FAIL("Invalid buffer provider: %s", s.c_str());
		}),
		Option("h|help", [&]()
		{
			puts(usage_str);
			exit(-1);
		}),
	};

	optionset.parse(argc, argv);

	if (optionset.params().size() > 0) {
		puts(usage_str);
		exit(-1);
	}

	auto pixfmt = PixelFormat::YUYV;
    //auto pixfmt = PixelFormat::NV12; //only after vpe

	Card card;

	auto conn = card.get_first_connected_connector();
	auto crtc = conn->get_current_crtc();
	printf("Display: %dx%d\n", crtc->width(), crtc->height());
	printf("Buffer provider: %s\n", buffer_provider == BufferProvider::V4L2? "V4L" : "DRM");

	vector<int> camera_fds;

	for (string vidpath : glob("/dev/video*")) {
		int fd = ::open(vidpath.c_str(), O_RDWR | O_NONBLOCK);

		if (fd < 0)
			continue;

		if (!is_capture_dev(fd)) {
			close(fd);
			continue;
		}

		if(camera_fds.size()==3)
			break;

		camera_fds.push_back(fd);

		printf("Using %s\n", vidpath.c_str());

		if (single_cam)
			break;
	}

	FAIL_IF(camera_fds.size() == 0, "No cameras found");

	vector<Plane*> available_planes;
	for (Plane* p : crtc->get_possible_planes()) {

		printf("id=%d plane_type=%d support_format=%d\n", (int)(p->id()), (int)(p->plane_type()), (int)(p->supports_format(pixfmt)));

		if (p->plane_type() != PlaneType::Overlay)
			continue;

		if (!p->supports_format(pixfmt))
			continue;

		available_planes.push_back(p);
	}

	printf("available_planes=%d camera_fds=%d\n", available_planes.size(), camera_fds.size());
	FAIL_IF(available_planes.size() < camera_fds.size(), "Not enough video planes for cameras");

	uint32_t plane_w = crtc->width() / camera_fds.size();
	vector<CameraPipeline*> cameras;

	for (unsigned i = 0; i < camera_fds.size(); ++i) {
		int cam_fd = camera_fds[i];
		Plane* plane = available_planes[i];

		auto cam = new CameraPipeline(cam_fd, card, crtc, plane, i * plane_w, 0,
					      plane_w, crtc->height(), pixfmt, buffer_provider, available_planes.size());
		cameras.push_back(cam);
	}

	unsigned nr_cameras = cameras.size();

	vector<pollfd> fds(nr_cameras + 1);

	for (unsigned i = 0; i < nr_cameras; i++) {
		fds[i].fd = cameras[i]->fd();
		fds[i].events =  POLLIN;
	}
	fds[nr_cameras].fd = 0;
	fds[nr_cameras].events =  POLLIN;

	for (auto cam : cameras)
		cam->start_streaming();

	while (true) {
		int r = poll(fds.data(), nr_cameras, -1);
		ASSERT(r > 0);

#if(1) // most best way for maxing fps
		AtomicReq req(card);

		for (unsigned i = 0; i < nr_cameras; i++)
		{

			if (!fds[i].revents)
				continue;
			cameras[i]->show_next_frame(req);
			req.commit_sync();
			fds[i].revents = 0;
		}
#else
		if (fds[nr_cameras].revents != 0)
			break;

		AtomicReq req(card);

		for (unsigned i = 0; i < nr_cameras; i++)
		{

			if (!fds[i].revents)
				continue;
			cameras[i]->show_next_frame(req);
			fds[i].revents = 0;
		}

		r = req.test();
		FAIL_IF(r, "Atomic commit failed: %d", r);

		req.commit_sync();
#endif
	}

	for (auto cam : cameras)
		delete cam;
}
