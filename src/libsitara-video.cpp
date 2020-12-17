#include "libsitara-video.h"

#define VIDEO_CAPTURE_CAMERA_BUF_QUEUE_SIZE		(6)

//#define TEST_SIMLE_VIDEO_CAP
//#define TEST_VIDEO_CAPTURE_USE_VPE_OMAP_DRM
//#define TEST_VIDEO_CAPTURE_USE_VIDEO_CAPTURE_CLASS
//#define MULTI_THREAD_TEST_VIDEO_CAPTURE_USE_VIDEO_CAPTURE_CLASS
//#define TEST_ALL_WRAP
#define TEST_VIDEO_DISPLAY

#if defined(TEST_SIMLE_VIDEO_CAP)
#include "videoenclib.h"
#include "simple_video_capture.h"

class ForTest: public VideoEncLib::AbstractInputBuffer
{
	explicit ForTest();
	virtual ~ForTest();
};

int main(int argc, char *argv[])
{
	svl::SimpleVideoCapture vid("/dev/video1", 704, 280, "yuyv", 704, 560, "yuyv"/*"nv12"*/, true, true);

	while(true)
	{
		vid.getFrame(nullptr);
	}

	return 0;
}
#elif defined(TEST_VIDEO_CAPTURE_USE_VPE_OMAP_DRM)
#include "drm_visual.h"
#include "video_capture.h"
#include "basic_func_v4l2.h"

#include <poll.h>
#include <math.h>

#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>

#include <chrono>
#include <thread>

const int bar_speed = 4;
const int bar_width = 10;

static unsigned get_bar_pos(kms::Framebuffer* fb, unsigned frame_num)
{
	return (frame_num * ::bar_speed) % (fb->width() - ::bar_width + 1);
}

static void countFrameVideo(Stopwatch &clock_, unsigned &cnt_frame, unsigned &history_cnt_frame, bool use_50_fps_)
{
	++cnt_frame;

	if(clock_.elapsed_ms() >= 1000.0)
	{
		if(cnt_frame > 50)
			cnt_frame = 50;
		else if (!use_50_fps_ && cnt_frame > 25 && cnt_frame < 27)
			cnt_frame = 25;

		if(history_cnt_frame != cnt_frame)
		{
			std::cout << "fps='" << cnt_frame << "'" << std::endl;
			history_cnt_frame = cnt_frame;
		}

		cnt_frame = 0;
		clock_.start();
	}
}

static void draw_text_on_frame(kms::Framebuffer* fb, unsigned cnt_frame)
{
	static std::map<kms::Framebuffer*, int> s_bar_pos_map;
	static uint32_t tik = 0;

	int old_pos = -1;
	if (s_bar_pos_map.find(fb) != s_bar_pos_map.end())
		old_pos = s_bar_pos_map[fb];

	int pos = ::get_bar_pos(fb, tik++);
	kms::draw_color_bar(*fb, old_pos, pos, ::bar_width);
	kms::draw_text(*fb, fb->width() / 2, 0, std::to_string(cnt_frame), kms::RGB(255, 255, 255));
	s_bar_pos_map[fb] = pos;
}

int main(int argc, char *argv[])
{
	const bool use_50_fps = /*false*/true;
	bool do_once	= false;

	svl::ListOfV4l2Dev ls;
	svl::ListOfV4l2Dev::DataAboutV4l2DeV lst = ls.getV4l2DevList();

	std::unique_ptr <VideoDevice> ptr_vc  = nullptr;
	std::unique_ptr <VideoDevice> ptr_vpe = nullptr;

	if( lst.str_.size() )
	{
		svl::DRMvisual::getInstance().initOmap(kms::PixelFormat::NV12/*YUYV*/);

		auto lambdaOpenVideoDevice = [](const std::string &str, std::unique_ptr <VideoDevice> &ptr)
		{
			try
			{
				std::cout << "open device '" << str << "'" << std::endl;

				ptr.reset(new VideoDevice(str));
			}
			catch(const std::exception &e)
			{
				std::cout << "was exception '" << e.what() << "'" << std::endl;
			}

			std::cout << "fd '" 		<< ptr->fd()			<< "'" << std::endl;
			std::cout << "has_capture '"<< ptr->has_capture()	<< "'" << std::endl;
			std::cout << "has_output '" << ptr->has_output()	<< "'" << std::endl;
			std::cout << "has_m2m '"	<< ptr->has_m2m()		<< "'" << std::endl;
		};

		lambdaOpenVideoDevice("/dev/video0", ptr_vpe);
		lambdaOpenVideoDevice("/dev/video1", ptr_vc);

		svl::DRMvisual::ListOfPlanesDef list_planes(svl::DRMvisual::getInstance().getAvailablePlanes());
		const kms::Connector	*ptr_con = svl::DRMvisual::getInstance().getConnector();
		const kms::Crtc 		*ptr_crt = svl::DRMvisual::getInstance().getCrtc();

		auto lambdaPrint = [](const svl::DRMvisual::ListOfPlanesDef &lst)
		{
			for (kms::Plane *p : lst)
				std::cout << "id='" <<	(int)(p->id()) << "' plane_type='" <<
				(int)(p->plane_type()) << "' support_format='" <<
				(int)(p->supports_format(svl::DRMvisual::getInstance().getPixFmt())) << "'" << std::endl;
		};
		//lambdaPrint(list_planes);

		VideoStreamer *in		= ptr_vc->get_capture_streamer();

		VideoStreamer *in_vpe	= ptr_vpe->get_capture_streamer(true);
		VideoStreamer *out_vpe	= ptr_vpe->get_output_streamer(true);

		struct v4l2_frmsizeenum frm_size = svl::BasicFuncV4l2::getFrameSizeV4l2Dev(in->fd(), kms::PixelFormat::YUYV);

		std::cout << "width_cam ='" << frm_size.discrete.width << "'" <<
		", height_cam ='" << frm_size.discrete.height << "'" << std::endl;

		in->set_format(kms::PixelFormat::YUYV, frm_size.discrete.width, frm_size.discrete.height);
		in->set_queue_size(VIDEO_CAPTURE_CAMERA_BUF_QUEUE_SIZE);

		in_vpe->set_format(kms::PixelFormat::YUYV, frm_size.discrete.width, frm_size.discrete.height, 1);
		in_vpe->set_queue_size(VIDEO_CAPTURE_CAMERA_BUF_QUEUE_SIZE);

		out_vpe->set_format(kms::PixelFormat::NV12/*YUYV*/, frm_size.discrete.width, frm_size.discrete.height);
		out_vpe->set_queue_size(VIDEO_CAPTURE_CAMERA_BUF_QUEUE_SIZE);

		for (unsigned i = 0; i < VIDEO_CAPTURE_CAMERA_BUF_QUEUE_SIZE; ++i)
		{
			kms::OmapCard *ptr = dynamic_cast<kms::OmapCard*>(svl::DRMvisual::getInstance().getCard());

			if(ptr != nullptr)
			{
				auto fb = new kms::OmapFramebuffer( *ptr, frm_size.discrete.width, frm_size.discrete.height, kms::PixelFormat::YUYV);
				in->queue(fb);
			}
		}

		for (unsigned i = 0; i < VIDEO_CAPTURE_CAMERA_BUF_QUEUE_SIZE; ++i)
		{
			kms::OmapCard *ptr = dynamic_cast<kms::OmapCard*>(svl::DRMvisual::getInstance().getCard());

			if(ptr != nullptr)
			{
				auto fb = new kms::OmapFramebuffer( *ptr, frm_size.discrete.width, frm_size.discrete.height, kms::PixelFormat::NV12/*YUYV*/);
				out_vpe->queue(fb);
			}
		}

		svl::DRMvisual::getInstance().initPlane(
												list_planes[0], out_vpe->getVideoBuffer()[0],
												svl::DRMvisual::OutXYDef(0, 0),
												svl::DRMvisual::OutWHDef(frm_size.discrete.width/2, frm_size.discrete.height),
												svl::DRMvisual::InWHDef(frm_size.discrete.width, frm_size.discrete.height)
												);

		std::vector<pollfd> fds_c(1);
		for (unsigned i = 0; i < fds_c.size(); ++i)
		{
			fds_c[i].fd = ptr_vc->fd();
			fds_c[i].events =  POLLIN;
		}

		std::vector<pollfd> fds_vpe(1);
		for (unsigned i = 0; i < fds_vpe.size(); ++i)
		{
			fds_vpe[i].fd = ptr_vpe->fd();
			fds_vpe[i].events =  POLLIN;
		}

		in->stream_on();
		in_vpe->stream_on();

		uint32_t frame_num = 0, last_frame_num = 0;

		Stopwatch clock;
		clock.start();

		unsigned count = 0;

		while(true)
		{
				try
				{
					uint32_t ind_of_buf = 0;
					uint32_t frame_order = 0;

					int r = poll(fds_c.data(), fds_c.size(), -1);
					ASSERT(r > 0);

					kms::OmapFramebuffer *dst_fb = dynamic_cast<kms::OmapFramebuffer*>(in->dequeue(&frame_order));

					if(do_once)
					{
						//std::this_thread::sleep_for(std::chrono::microseconds(10));
						in_vpe->dequeue();
					}

					in_vpe->queue(dst_fb, &frame_order);

					if (!do_once)
					{
						++count;
						for (unsigned i = 1; i <= VIDEO_CAPTURE_CAMERA_BUF_QUEUE_SIZE; i++)
						{
							// To star deinterlace, minimum 3 frames needed
							if (count != 3)
							{
								r = poll(fds_c.data(), fds_c.size(), -1);
								ASSERT(r > 0);

								dst_fb = dynamic_cast<kms::OmapFramebuffer*>(in->dequeue(&frame_order));
								in_vpe->queue(dst_fb, &frame_order);
							}
							else
							{
								out_vpe->stream_on();
								do_once = true;
								std::cout << "streaming started..." << std::endl;
								break;
							}
							++count;
						}
					}

					r = poll(fds_vpe.data(), fds_vpe.size(), -1);
					ASSERT(r > 0);

					kms::OmapFramebuffer *dst_fb_out = dynamic_cast<kms::OmapFramebuffer*>(out_vpe->dequeue());

#if(1)
					if(use_50_fps || frame_order != V4L2_FIELD_BOTTOM)
					{
						::countFrameVideo(clock, frame_num, last_frame_num, use_50_fps);

						::draw_text_on_frame(dst_fb_out, last_frame_num);

						svl::DRMvisual::getInstance().addFbIdToPlane(list_planes[0], dst_fb_out);
					}
#endif

					in->queue(dst_fb);
					out_vpe->queue(dst_fb_out);
				}
				catch (std::system_error &se)
				{
					if (se.code() != std::errc::resource_unavailable_try_again)
						FAIL("dequeue failed: %s", se.what());

					break;
				}

		}

		std::cout << "exiting..." << std::endl;
	}

	return 0;
}
#elif defined(TEST_VIDEO_CAPTURE_USE_VIDEO_CAPTURE_CLASS)
#include "video_capture.h"

int main(int argc, char *argv[])
{
	svl::ListOfV4l2Dev::DataAboutV4l2DeV	lst_dev_v4l2(svl::VideoCapture::getListDev());

#if(1)
	kms::PixelFormat my_px_fmt = kms::PixelFormat::NV12;
#else
	kms::PixelFormat my_px_fmt = kms::PixelFormat::YUYV;
#endif

	svl::DRMvisual::getInstance().initOmap(my_px_fmt);
	const svl::DRMvisual::ListOfPlanesDef 	lst_plane(svl::VideoCapture::getAvailablePlanesOfDrm());

	svl::VideoCapture 						cap(
												lst_dev_v4l2.str_[0], lst_plane[0], 7, true, true, true, true,
												svl::DRMvisual::OutXYDef(0, 0),
												svl::DRMvisual::OutWHDef(704/2, 280),
												my_px_fmt, true, true
												);

	cap.startCapture();

	while(true)
	{
		cap.getFrame();
		cap.continueCapture();
	}

	cap.stopCapture();
}
#elif defined(MULTI_THREAD_TEST_VIDEO_CAPTURE_USE_VIDEO_CAPTURE_CLASS)

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <array>
#include <functional>

#include <signal.h>
#include <pthread.h>

#include "video_capture.h"
#include "nix_console.h"

std::mutex    			g_lock;
std::condition_variable g_check;
std::atomic<bool> 		g_done{ false };

// handler for event to signal SIGINT
static void sigHandler(int signo)
{
  if (signo == SIGINT)
  {
	  std::cout << "-- catch SIGINT" << std::endl;

	  std::cout << "-- catch SIGINT END" << std::endl;
  }
}

int registerHandlerForSignal(void)
{
	struct sigaction sa;
	sa.sa_handler = sigHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		std::cout << "Did not catch  SIGINT" << std::endl;
		return -1;
	}

	signal(SIGINT,sigHandler);
	return 0;
}

void threadConsole(int &id, std::string& str_)
{
	int id_ = id;

	::registerAllCommands();
	::workWithConsole();

	g_done = true;
	std::cout << "threadConsole id'"<< id_ <<"' ended" << std::endl;

	//g_check.notify_one();
	//g_check.notify_all();
	std::unique_lock<std::mutex> locker(g_lock);
	std::notify_all_at_thread_exit(g_check, std::move(locker));
}

void threadVideo(int &id, svl::VideoCaptureWrapper& cap)
{
	int id_ = id;

	//cap.startCapture();
	cap.start();

	unsigned tik = 0;
	while(true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		//cap.getFrame();
		VideoEncLib::IFrameBuffer* ptr= cap.waitFrame();

		if (!(tik++%253))
		{
			std::cout << "tik='"<< tik-1 << "'" << std::endl;
			std::cout << "y-size   ='"	<< ptr->yBuff()->size()		<< "'" << std::endl;
			std::cout << "y-dma-id ='"	<< ptr->yBuff()->dmaBuf()	<< "'" << std::endl;
			std::cout << "y-map    ='"	<< ptr->yBuff()->dataMap()	<< "'" << std::endl;

			std::cout << "uv-size   ='"	<< ptr->uvBuff()->size()	<< "'" << std::endl;
			std::cout << "uv-dma-id ='"	<< ptr->uvBuff()->dmaBuf()	<< "'" << std::endl;
			std::cout << "uv-map    ='"	<< ptr->uvBuff()->dataMap()	<< "'" << std::endl;
		}

		//cap.continueCapture();
		cap.endingProcessFrameNotify();

		{
		  std::unique_lock<std::mutex> locker(g_lock);

		  auto now_time = std::chrono::system_clock::now();

		  if( g_check.wait_until(locker, now_time + std::chrono::milliseconds(2), [](){ return 1; }))
		  {
			  if(g_done)
			  {
				  break;
			  }
		  }
		}
	}

	//cap.stopCapture();
	cap.stop();

	std::cout << "threadVideo id='"<< id_ << "' ended" << std::endl;

	//g_check.notify_all();
	std::unique_lock<std::mutex> locker(g_lock);
	std::notify_all_at_thread_exit(g_check, std::move(locker));
}

int main(int argc, char *argv[])
{
	registerHandlerForSignal();

	svl::ListOfV4l2Dev::DataAboutV4l2DeV	lst_dev_v4l2(svl::VideoCapture::getListDev());

#if(1)
	kms::PixelFormat my_px_fmt = kms::PixelFormat::NV12;
#else
	kms::PixelFormat my_px_fmt = kms::PixelFormat::YUYV;
#endif

	svl::DRMvisual::getInstance().initOmap(my_px_fmt);
	const svl::DRMvisual::ListOfPlanesDef 	lst_plane(svl::VideoCapture::getAvailablePlanesOfDrm());

	std::array<svl::VideoCaptureWrapper*, 4> cap_mas { nullptr, nullptr, nullptr, nullptr };

#define TEST_VIDEO_COUNT  (1)

#if TEST_VIDEO_COUNT>=1
	int tik = 0;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
//													lst_dev_v4l2.str_[tik], lst_plane[tik], 8, true, false, true, true,
//													svl::Rectangle<uint32_t>(0,     0,     704/2, 280),
//													svl::Rectangle<uint32_t>(0,     0,     704,   280),
//													my_px_fmt, true, true
												);
#endif
#if TEST_VIDEO_COUNT>=2
	++tik;
	cap_mas[tik] = new svl::VideoCapture(
											lst_dev_v4l2.str_[tik], lst_plane[tik], 8, true, false, true, true,
											svl::Rectangle<uint32_t>(704/2, 0,     704/2, 280),
											svl::Rectangle<uint32_t>(4*100, 4*30,  704/2, 280/2),
											my_px_fmt, true, true
										);
#endif
#if TEST_VIDEO_COUNT>=3
	++tik;
	cap_mas[tik] = new svl::VideoCapture(
											lst_dev_v4l2.str_[tik], lst_plane[tik], 8, true, false, true, true,
											svl::Rectangle<uint32_t>(0,     280,   704/2, 280),
											svl::Rectangle<uint32_t>(0,     0,     704,   280),
											my_px_fmt, true, true
										);
#endif
#if TEST_VIDEO_COUNT>=4
	++tik;
	cap_mas[tik] = new svl::VideoCapture(
											lst_dev_v4l2.str_[tik], lst_plane[tik], 8, true, false, false, true,
											svl::Rectangle<uint32_t>(0,     0,     704/2, 280),
											svl::Rectangle<uint32_t>(0,     0,     704,   280),
											my_px_fmt, true, true
										);
#endif

	int thr_id{0};
	std::string string_from_console;

	std::thread thr_console(::threadConsole, std::ref(thr_id), std::ref(string_from_console));
	++thr_id;

	int num_of_cap = 0;
#if TEST_VIDEO_COUNT>=1
	std::thread thr_video1(::threadVideo, std::ref(thr_id), std::ref(*cap_mas[num_of_cap++]));
#endif
#if TEST_VIDEO_COUNT>=2
	++thr_id;
	std::thread thr_video2(::threadVideo, std::ref(thr_id), std::ref(*cap_mas[num_of_cap++]));
#endif
#if TEST_VIDEO_COUNT>=3
	++thr_id;
	std::thread thr_video3(::threadVideo, std::ref(thr_id), std::ref(*cap_mas[num_of_cap++]));
#endif
#if TEST_VIDEO_COUNT>=4
	++thr_id;
	std::thread thr_video4(::threadVideo, std::ref(thr_id), std::ref(*cap_mas[num_of_cap]));
#endif

	// enable killing thread
	int res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if (res != 0)
		std::cout << "error-'" << res << "' pthread_setcancelstate" << std::endl;

	pthread_t id_console_thread = thr_console.native_handle();

	thr_console.join();

#if TEST_VIDEO_COUNT>=1
	thr_video1.join();
#endif
#if TEST_VIDEO_COUNT>=2
	thr_video2.join();
#endif
#if TEST_VIDEO_COUNT>=3
	thr_video3.join();
#endif
#if TEST_VIDEO_COUNT>=4
	thr_video4.join();
#endif

	for( int tik = 0; tik < cap_mas.size(); ++tik)
	{
		if(cap_mas[tik])
		{
			delete cap_mas[tik];
			cap_mas[tik] = nullptr;
		}
	}

	std::cout << "programm successfully ended" << std::endl;

	return 0;
}
#elif defined(TEST_ALL_WRAP)
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <array>

#include <pthread.h>

#include "libsitara-video.h"
#include "nix_console.h"

std::mutex    			g_lock;
std::condition_variable g_check;
std::atomic<bool> 		g_done{ false };

void threadConsole(const int id, std::string& str_)
{
	int id_ = id;

	::workWithConsole();

	g_done = true;
	std::cout << "threadConsole id'"<< id_ <<"' ended" << std::endl;

	//g_check.notify_one();
	//g_check.notify_all();
	std::unique_lock<std::mutex> locker(g_lock);
	std::notify_all_at_thread_exit(g_check, std::move(locker));
}

void threadVideo(const int id, svl::VideoCaptureWrapper& cap, const int num, const svl::DRMvisual::ListOfPlanesDef &lst_plans)
{
	int id_ = id;
	int num_ = num;
	//cap.startCapture();
	cap.start();

	unsigned tik__ = 0;
	bool first_shoot = true;
	VideoEncLib::Rectangle rec_d(0,0,0,0);
	VideoEncLib::Rectangle rec_v(0,0,0,0);

	while(true)
	{
		VideoEncLib::IFrameBuffer *ptr = nullptr;
		//cap.getFrame();
		if(num)
		{
			ptr = cap.waitFrame();
		}
		else
		{
			ptr= cap.waitFrame();
		}

#if(1)
		auto ptr_cur_plane = cap.getPlane();


		if(first_shoot)
		{
			switch (num_)
			{
				case 0:
					rec_d = VideoEncLib::Rectangle(0,		0,		704/2,	280);
					rec_v = VideoEncLib::Rectangle(0,		0,		704,	280);
					break;
				case 1:
					rec_d = VideoEncLib::Rectangle(704/2,	0,		704/2,	280);
					rec_v = VideoEncLib::Rectangle(0,		0,  	704,	280);
					break;
				case 2:
					rec_d = VideoEncLib::Rectangle(0,		280,   704/2,	280);
					rec_v = VideoEncLib::Rectangle(0,		0,     704,		280);
					break;

				default:
					break;
			}

			if (num_<3 && !cap.getUseDisplayed())
				svl::DRMvisual::getInstance().initPlane((ptr_cur_plane)?ptr_cur_plane:lst_plans[num_], ptr, rec_d, rec_v);

			first_shoot = false;
		}
		else
		{
			if (num_<3 && !cap.getUseDisplayed())
			{
				svl::DRMvisual::getInstance().addFbIdToPlane((ptr_cur_plane)?ptr_cur_plane:lst_plans[num_], ptr);
			}
		}
#endif
		if (!(tik__++%253))
		{
			std::cout << "-----------num='"<< num_ << "'-----------" << std::endl;
			std::cout << "tik='"<< tik__-1 << "'" << std::endl;
			std::cout << "y-size   ='"	<< ptr->yBuff()->size()		<< "'" << std::endl;
			std::cout << "y-dma-id ='"	<< ptr->yBuff()->dmaBuf()	<< "'" << std::endl;
			std::cout << "y-map    ='"	<< ptr->yBuff()->dataMap()	<< "'" << std::endl;

			std::cout << "uv-size   ='"	<< ptr->uvBuff()->size()	<< "'" << std::endl;
			std::cout << "uv-dma-id ='"	<< ptr->uvBuff()->dmaBuf()	<< "'" << std::endl;
			std::cout << "uv-map    ='"	<< ptr->uvBuff()->dataMap()	<< "'" << std::endl;
		}

		//cap.continueCapture();
		cap.endingProcessFrameNotify();

		{
		  std::unique_lock<std::mutex> locker(g_lock);

		  auto now_time = std::chrono::system_clock::now();

		  if( g_check.wait_until(locker, now_time + std::chrono::milliseconds(1), [](){ return 1; }))
		  {
			  if(g_done)
			  {
				  break;
			  }
		  }
		}
	}

	//cap.stopCapture();
	cap.stop();

	std::cout << "threadVideo id='"<< id_ << "' ended" << std::endl;

	//g_check.notify_all();
	std::unique_lock<std::mutex> locker(g_lock);
	std::notify_all_at_thread_exit(g_check, std::move(locker));
}

int main(int argc, char *argv[])
{
	svl::ListOfV4l2Dev::DataAboutV4l2DeV	lst_dev_v4l2(svl::VideoCapture::getListDev());

#if(1)
	kms::PixelFormat my_px_fmt = kms::PixelFormat::NV12;
#else
	kms::PixelFormat my_px_fmt = kms::PixelFormat::YUYV;
#endif

	svl::DRMvisual::getInstance().initOmap(my_px_fmt);
	const svl::DRMvisual::ListOfPlanesDef 	lst_plane(svl::VideoCapture::getAvailablePlanesOfDrm());

	std::array<svl::VideoCaptureWrapper*, 4> cap_mas { nullptr, nullptr, nullptr, nullptr };

#define TEST_VIDEO_COUNT  (4)

#if TEST_VIDEO_COUNT>=1
	int tik = 0;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												,true, true, 9, true, lst_plane[tik],
												VideoEncLib::Rectangle(0, 0, 704/2, 280),
												VideoEncLib::Rectangle(0, 0, 704,   280),
												false, true, true
												);
#endif
#if TEST_VIDEO_COUNT>=2
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												,true, true, 9, false, lst_plane[tik],
												VideoEncLib::Rectangle(704/2, 0, 704/2,280),
												VideoEncLib::Rectangle(0,     0, 704,  280),
												false, true, false, my_px_fmt
												);
#endif
#if TEST_VIDEO_COUNT>=3
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												,true, true, 9, false
												);
#endif
#if TEST_VIDEO_COUNT>=4
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												);
#endif

	int thr_id{0};
	std::string string_from_console;

	std::thread thr_console(::threadConsole, thr_id, std::ref(string_from_console));
	++thr_id;

	int num_of_cap = 0;
#if TEST_VIDEO_COUNT>=1
	std::thread thr_video1(::threadVideo, thr_id, std::ref(*cap_mas[num_of_cap]), num_of_cap, std::cref(lst_plane));
#endif
#if TEST_VIDEO_COUNT>=2
	++thr_id, ++num_of_cap;
	std::thread thr_video2(::threadVideo, thr_id, std::ref(*cap_mas[num_of_cap]), num_of_cap, std::cref(lst_plane));
#endif
#if TEST_VIDEO_COUNT>=3
	++thr_id, ++num_of_cap;
	std::thread thr_video3(::threadVideo, thr_id, std::ref(*cap_mas[num_of_cap]), num_of_cap, std::cref(lst_plane));
#endif
#if TEST_VIDEO_COUNT>=4
	++thr_id, ++num_of_cap;
	std::thread thr_video4(::threadVideo, thr_id, std::ref(*cap_mas[num_of_cap]), num_of_cap, std::cref(lst_plane));
#endif

	// enable killing thread
	int res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if (res != 0)
		std::cout << "error-'" << res << "' pthread_setcancelstate" << std::endl;

	pthread_t id_console_thread = thr_console.native_handle();

	thr_console.join();

#if TEST_VIDEO_COUNT>=1
	thr_video1.join();
#endif
#if TEST_VIDEO_COUNT>=2
	thr_video2.join();
#endif
#if TEST_VIDEO_COUNT>=3
	thr_video3.join();
#endif
#if TEST_VIDEO_COUNT>=4
	thr_video4.join();
#endif

	for( int tik = 0; tik < cap_mas.size(); ++tik)
	{
		if(cap_mas[tik])
		{
			delete cap_mas[tik];
			cap_mas[tik] = nullptr;
		}
	}

	std::cout << "programm successfully ended" << std::endl;

	return 0;
}
#elif defined(TEST_VIDEO_DISPLAY)
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <array>
#include <limits>

#include <signal.h>
#include <pthread.h>

#include "libsitara-video.h"
#include "nix_console.h"

std::mutex    			g_lock;
std::condition_variable g_check;
std::atomic<bool> 		g_done{ false };

std::array<svl::VideoCaptureWrapper*, 4>	cap_mas		{ nullptr, nullptr, nullptr, nullptr };
std::array<svl::VideoDisplay*, 3>			disp_mas	{ nullptr, nullptr, nullptr};

// handler for event to signal SIGINT
static void sigHandler(int signo)
{
  if (signo == SIGINT)
  {
	  std::cout << "-- catch SIGINT" << std::endl;

	  std::cout << "-- catch SIGINT END" << std::endl;
  }
}

int registerHandlerForSignal(void)
{
	struct sigaction sa;
	sa.sa_handler = sigHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		std::cout << "Did not catch  SIGINT" << std::endl;
		return -1;
	}

	signal(SIGINT,sigHandler);
	return 0;
}

int getNumFromCmd(int max, int min, char *args)
{
	char *tok;
	int num_of_cap = 0;

	int args_num = ::get_token_num(args, cm_delim);
	if (args_num != 1)
	{
		printf("error\n");
		return std::numeric_limits<int>::min();
	}
	else
	{
		num_of_cap = ::atoi(::strtok(args, cm_delim));

		if(num_of_cap > max)
			num_of_cap = max;
		else if(num_of_cap < min)
			num_of_cap =min;
	}

	return num_of_cap;
}

int cm_start_cap(char *args)
{
	int num_of_cap = getNumFromCmd(3, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(cap_mas[num_of_cap])
		cap_mas[num_of_cap]->start();

	printf("start capture %d\n", num_of_cap);
}

int cm_stop_cap(char *args)
{
	int num_of_cap = getNumFromCmd(3, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(cap_mas[num_of_cap])
		cap_mas[num_of_cap]->stop();

	printf("stop capture %d\n", num_of_cap);
}

int cm_on_display(char *args)
{
	int num_of_cap = getNumFromCmd(2, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(disp_mas[num_of_cap])
		disp_mas[num_of_cap]->on();

	printf("on display %d\n", num_of_cap);
}

int cm_of_display(char *args)
{
	int num_of_cap = getNumFromCmd(2, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(disp_mas[num_of_cap])
		disp_mas[num_of_cap]->off();

	printf("off display %d\n", num_of_cap);
}

int cm_start_disp(char *args)
{
	int num_of_cap = getNumFromCmd(2, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(disp_mas[num_of_cap])
		disp_mas[num_of_cap]->start();

	printf("start display %d\n", num_of_cap);
}

int cm_stop_disp(char *args)
{
	int num_of_cap = getNumFromCmd(2, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(disp_mas[num_of_cap])
		disp_mas[num_of_cap]->stop();

	printf("stop display %d\n", num_of_cap);
}

void threadConsole(const int id, std::string& str_, std::array<svl::VideoDisplay*, 3> &mas, std::array<svl::VideoCaptureWrapper*, 4> &cap)
{
	int id_ = id;

	unsigned tik = 0;
	for(auto wtf: mas)
	{
		if(wtf)
		{
			wtf->start();
			wtf->on();
			//cap[tik++]->start();
		}

	}

	::registerAllCommands();
	::workWithConsole();

	tik = 0;
	for(auto wtf: mas)
	{
		cap[tik++]->stop();
		//std::this_thread::sleep_for(std::chrono::milliseconds(50));
		if(wtf)
			wtf->stop();
	}

	g_done = true;
	std::cout << "threadConsole id'"<< id_ <<"' ended" << std::endl;

	//g_check.notify_one();
	//g_check.notify_all();
	std::unique_lock<std::mutex> locker(g_lock);
	std::notify_all_at_thread_exit(g_check, std::move(locker));
}

int main(int argc, char *argv[])
{
	svl::ListOfV4l2Dev::DataAboutV4l2DeV	lst_dev_v4l2(svl::VideoCapture::getListDev());

	svl::DRMvisual::getInstance().initOmap(kms::PixelFormat::NV12);

#define TEST_VIDEO_COUNT  (3)

#if TEST_VIDEO_COUNT>=1
	int tik = 0;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												,true, true, 9, false
												);
	disp_mas[tik] = new svl::VideoDisplay(cap_mas[tik]);
	disp_mas[tik]->setViewDisplayRect	(VideoEncLib::Rectangle(0, 0, 704/2, 280));
	disp_mas[tik]->setViewVideoRect		(VideoEncLib::Rectangle(0, 0, 704,   280));

#endif
#if TEST_VIDEO_COUNT>=2
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												,true, true, 9, false
												);
	disp_mas[tik] = new svl::VideoDisplay(cap_mas[tik]);
	disp_mas[tik]->setViewDisplayRect	(VideoEncLib::Rectangle(704/2, 0, 704/2,280));
	disp_mas[tik]->setViewVideoRect		(VideoEncLib::Rectangle(0,     0, 704,  280));
#endif
#if TEST_VIDEO_COUNT>=3
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												,true, true, 9, false
												);
	disp_mas[tik] = new svl::VideoDisplay(cap_mas[tik]);
	disp_mas[tik]->setViewDisplayRect	(VideoEncLib::Rectangle(0,		280,   704/2,	280));
	disp_mas[tik]->setViewVideoRect		(VideoEncLib::Rectangle(0,		0,     704,		280));
#endif

	int thr_id{0};
	std::string string_from_console;

	std::thread thr_console(::threadConsole, thr_id, std::ref(string_from_console), std::ref(disp_mas), std::ref(cap_mas));
	++thr_id;

	thr_console.join();

	std::cout << "programm successfully ended" << std::endl;

	return 0;
}
#endif
#undef VIDEO_CAPTURE_CAMERA_BUF_QUEUE_SIZE
