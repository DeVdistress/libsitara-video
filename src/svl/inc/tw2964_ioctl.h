/*
 * tw2964_reg.h
 *
 *  Created on: 07 авг. 2019 г.
 *      Author: DeVdistress
 */

#ifndef TW2964_IOCTL_H_
#define TW2964_IOCTL_H_

//------------------------------------------------------------------------------
/// @brief Тип данных номера видео для v4l2
//------------------------------------------------------------------------------
typedef enum Tw2964VideoNumOfV4l2Enum
{
	Video1Enum = 1, //!< номер канала 1 >
	Video2Enum,		//!< номер канала 2 >
	Video3Enum,		//!< номер канала 3 >
	Video4Enum,		//!< номер канала 4 >
	VideoMaxCntEnum	//!< максимальное кол-во каналов поддерживаемое в драйвере >
} Tw2964VideoNumOfV4l2TypeDef;

//------------------------------------------------------------------------------
/// @brief Тип данных состояние видео
//------------------------------------------------------------------------------
typedef struct
{
	Tw2964VideoNumOfV4l2TypeDef num_channel;  	//[in]	!< номер канала, !!!выставляется из user space!!! >
	int detect_video;							//[out]	!< признак обнаружения виде на входе tw2964>
	int mono_video;								//[out]	!< признак чёрно-белого видео tw2964>
	int det50;									//[out]	!< признак обнаружения 50Гц кадровой частоты>
}Tw2964StatVideoTypeDef;

//------------------------------------------------------------------------------
/// @brief Тип данных номера аналогового видео входа tw2964
//------------------------------------------------------------------------------
typedef enum Tw2964AnalogVideoInputNumEnum
{
	AnalogVideoIn1Enum, 	//!< номер канала 1 >
	AnalogVideoIn2Enum,		//!< номер канала 2 >
	AnalogVideoIn3Enum,		//!< номер канала 3 >
	AnalogVideoIn4Enum,		//!< номер канала 4 >
	AnalogVideoInMaxCntEnum	//!< максимальное кол-во каналов поддерживаемое в драйвере >
} Tw2964AnalogVideoInputNumTypeDef;

//------------------------------------------------------------------------------
/// @brief Тип данных для номера ревизии am5728
//------------------------------------------------------------------------------
typedef enum RevOfAm5728NumEnum
{
	SR1_0Enum,	//!< номер ревизии >
	SR1_1Enum,
	SR2_0Enum,
	UnknownRevEnum
} RevOfAm5728NumTypeDef;


//------------------------------------------------------------------------------
/// @brief Тип данных соответствия номера v4l2 и аналогового видео входа tw2964
//------------------------------------------------------------------------------
typedef struct
{
	Tw2964VideoNumOfV4l2TypeDef			v4l2_num;  	//[out]	!< номер канала v4l2 типа /dev/video1../dev/video4 >
	Tw2964AnalogVideoInputNumTypeDef	ain_num;	//[out]	!< номер канала аналогового видео входа tw2964>
}Tw2964AccordanceV4l2AndAnalogInTypeDef;

//------------------------------------------------------------------------------
/// @brief Тип данных счётчик прерываний
//------------------------------------------------------------------------------
typedef struct
{
	Tw2964VideoNumOfV4l2TypeDef num_channel;  		//[in]	!< номер канала, !!!выставляется из user space!!! >
	unsigned int int_count;							//[out]	!< кол-во прерываний на канале num_channel>
}Tw2964StatIrqTypeDef;

//------------------------------------------------------------------------------
/// @brief Тип данных стандарт видео декодера
//------------------------------------------------------------------------------
typedef enum Tw2964VideoStdEnum
{
	PalVideoStdEnum, 	//!< ПАЛ >
	NtscVideoStdEnum,	//!< НТС >
	VideoStdMaxCntEnum	//!< максимальное кол-во каналов поддерживаемое в драйвере >
}Tw2964VideoStdTypeDef;

//------------------------------------------------------------------------------
/// @brief Тип данных канал и тип декодера
//------------------------------------------------------------------------------
typedef struct
{
	Tw2964VideoNumOfV4l2TypeDef num_channel;  	//[in]	!< номер канала, !!!выставляется из user space!!! >
	Tw2964VideoStdTypeDef		video_std;		//[out]	!< тип декодера>
}Tw2964ChanVideoStdTypeDef;

//------------------------------------------------------------------------------
/// @brief Тип данных канал, width и high кадра
//------------------------------------------------------------------------------
typedef struct
{
	Tw2964VideoNumOfV4l2TypeDef num_channel;  	//[in]	!< номер канала, !!!выставляется из user space!!! >
	unsigned int				width;			//[out]	!< ширина кадра>
	unsigned int				high;			//[out]	!< вышина кадра>
}Tw2964ChanWidthHighTypeDef;

//------------------------------------------------------------------------------
/// @brief IOCTL Получение кол-ва каналов
//------------------------------------------------------------------------------
#define GET_COUNT_OF_CHANNEL_TW2964						_IO(IOC_MAGIC_TW2964, 1)

//------------------------------------------------------------------------------
/// @brief IOCTL Получение cоответствия номера видео выданного v4l2 и аналогового видео входа tw2964
//------------------------------------------------------------------------------
#define GET_ACCORDANCE_VIDEO_NUM_V4L2_AND_ANALOG_INPUT	_IOWR(IOC_MAGIC_TW2964, 2, Tw2964AccordanceV4l2AndAnalogInTypeDef*)

//------------------------------------------------------------------------------
/// @brief IOCTL Получение состояния видео по каналу (см. Tw2964StatVideoTypeDef)
//------------------------------------------------------------------------------
#define GET_STATE_VIDEO_DETECT							_IOWR(IOC_MAGIC_TW2964, 3, Tw2964StatVideoTypeDef*)

//------------------------------------------------------------------------------
/// @brief IOCTL Получение кол-ва прерывания по каналу (см. Tw2964StatIrqTypeDef)
//------------------------------------------------------------------------------
#define GET_COUNT_IRQ_OF_CHANNEL						_IOWR(IOC_MAGIC_TW2964, 4, Tw2964StatIrqTypeDef*)

//------------------------------------------------------------------------------
/// @brief IOCTL Получение кол-ва прерывания по каналу (см. Tw2964StatIrqTypeDef)
//------------------------------------------------------------------------------
#define SET_COLOR_KILLER								_IOW(IOC_MAGIC_TW2964, 5, unsigned char*)

//------------------------------------------------------------------------------
/// @brief IOCTL Получение уровня для COLOR KILLER (0 - 63) меньше <=> сильнее
//------------------------------------------------------------------------------
#define GET_COLOR_KILLER								_IO(IOC_MAGIC_TW2964, 6)

//------------------------------------------------------------------------------
/// @brief IOCTL Установка стандарта для конкретного канала
//------------------------------------------------------------------------------
#define SET_VIDEO_STD_TW2964							_IOW(IOC_MAGIC_TW2964, 7, Tw2964ChanVideoStdTypeDef*)

//------------------------------------------------------------------------------
/// @brief IOCTL Получение стандарта для конкретного канала
//------------------------------------------------------------------------------
#define GET_VIDEO_STD_TW2964							_IOWR(IOC_MAGIC_TW2964, 8, Tw2964ChanVideoStdTypeDef*)

//------------------------------------------------------------------------------
/// @brief IOCTL Получение стандарта для конкретного канала с дитектора микросхемы
//------------------------------------------------------------------------------
#define GET_REAL_VIDEO_STD_TW2964						_IOWR(IOC_MAGIC_TW2964, 9, Tw2964ChanVideoStdTypeDef*)

//------------------------------------------------------------------------------
/// @brief IOCTL Получение формата кадра для конкретного канала
//------------------------------------------------------------------------------
#define GET_VIDEO_WIDTH_HIGH_TW2964						_IOWR(IOC_MAGIC_TW2964, 10, Tw2964ChanWidthHighTypeDef*)

//------------------------------------------------------------------------------
/// @brief IOCTL Актуализация стандарта внутри драйвера и микросхемного детектора
///
///		рекомендуется делать перед каждым использованием драйвера взамен ручного
///		использования GET_VIDEO_STD_TW2964 и GET_REAL_VIDEO_STD_TW2964
//------------------------------------------------------------------------------
#define ACTUALIZATION_STD_TW2964						_IO(IOC_MAGIC_TW2964, 11)

//------------------------------------------------------------------------------
/// @brief IOCTL Получение ревизии кристалла sitara
//------------------------------------------------------------------------------
#define GET_REVIZION_SITARA_AM5728						_IO(IOC_MAGIC_TW2964, 12)

// @brief	Служебные параметры
//			IOCTL
#define IOC_MAGIC_TW2964 								't'

#endif // TW2964_IOCTL_H_
