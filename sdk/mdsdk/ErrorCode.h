#ifndef ERRORCODE_H
#define ERRORCODE_H

/**

???               ????          ???
Public(??)       1-1000            E_PUB_
DataPool          1001-2000         E_DBP_
Logic             2001-3000         E_LGC_
DeviceManager     3001-4000         E_DEV_
ServiceManager    4001-5000         E_SVR_
Business          5001-6000         E_BUS_
Client            6001-7000         E_CLI_
Admin             7001-8000         E_ADM_
UserManager       8001-9000         E_USR_
PlaybackTrack     9001-10000        E_PBT_

*/

enum eVasErrorCode
{

	SUCCESS = 0,
	E_CLI_INIT_ALREADYDONE = 1001,   										//?????
	E_CLI_INIT_NOTYETDONE  = 1002,  										//??????
	E_CLI_CREATE_THREAD    = 1003,   										//??????
	E_CLI_COMM_CONNECT =     1101,											//???????
	E_CLI_NOT_LOGIN =        1102,   										//???????
	E_CLI_INVALID_PARAM =    1103,   										//?????
	E_CLI_NOTSET_STREAMCALLBACK = 1104, 									//??????????
	E_CLI_NOTSET_LARGEDATACALLBACK = 1105, 									//??????????????

 
    /*ResourceLogic*/
    E_LGC_PARAM_ERROR                                                 = 2001,//????
    E_LGC_OP_DB_ERROR                                                 = 2002,//???????
    E_LGC_NO_PRIVILEGE                                                = 2003,//?????
    E_LGC_NO_SUCH_RECORD                                              = 2004,//?????
    E_LGC_CREATE_RECORD                                               = 2005,//??????

    /*PlaybackTrackLogic*/
    E_LGC_PBT_SERVICEMANAGER_INVALID                                  = 2101,//ServiceManager????????


    /*ScheduleLogic*/
    E_SCHEDULE_INIT_PARAM_INVALID                                     = 2201,//???????
    E_SCHEDULE_DATAPOOL_POINTER_INVALID                               = 2202,//???????
    E_SCHEDULE_SERVICEMANGER_POINTER_INVALID                          = 2203,//????????
    E_SCHEDULE_DEVICEMANGER_POINTER_INVALID                           = 2204,//????????
    E_SCHEDULE_BUSINESS_POINTER_INVALID                               = 2205,//??????
    E_SCHEDULE_PROTO_PRC_HEAD_POINTER_INVALID                         = 2206,//???????
    E_SCHEDULE_PROTO_PRC_BODY_POINTER_INVALID                         = 2207,//???????
	E_SCHEDULE_INVALID_BODY_POINTER                                   = 2210,//???????


    /*AdminLogic*/
    //error code starts from 2301
	E_LGC_ADMIN_HAD_LOGIN											  = 2301,//??????????

    /*BaseLogic*/
    //error code starts from 2401

    /*DataProcessLogic*/
    //error code starts from 2501


    /*StatusLogic*/
    //error code starts from 2601

    /*Logic*/
    //error code starts from 2901
    E_LGC_NOT_HANDLED_BY_RESOURCELOGIC                                = 2901,//????ResourceLogic??
    E_LGC_NOT_HANDLED_BY_PLAYBACKTRACKLOGIC                           = 2902,//????PlaybackTrackLogic??
    E_LGC_NOT_HANDLED_BY_SCHEDULELOGIC                                = 2903,//????ScheduleLogic??
    E_LGC_NOT_HANDLED_BY_ADMINLOGIC                                   = 2904,//????AdminLogic??
    E_LGC_NOT_HANDLED_BY_BASELOGIC                                    = 2905,//????BaseLogic??
    E_LGC_NOT_HANDLED_BY_DATAPROCESSLOGIC                             = 2906,//????AdminLogic??
    E_LGC_NOT_HANDLED_BY_STATUSLOGIC                                  = 2907,//????StatusLogic??
    
    E_LGC_NOT_HANDLED_BY_BUSINESSLOGIC                                = 2908,//????BusinessLogic??
    E_LGC_NOT_HANDLED_BY_SHROADLOGIC                                  = 2909,//????ShroadLogic??
    
    E_LGC_NOT_HANDLED_BY_ALL_LOGIC                                    = 2910,//???????Logic??

    /*DeviceManager*/
    E_DEV_COMM_ERROR                                                  = 3001,//??Middleware??
    E_DEV_THREAD_ERROR                                                = 3101,//????????
 

    /*ServiceManager*/
    E_SER_PARAM_ERROR                                                 = 4001,//??????
    E_SER_MEMORY_ERROR                                                = 4002,//??????
    E_SER_GET_MODULE_ERROR                                            = 4003,//??????
    E_SER_START_SERVER_ERROR                                          = 4004,//??????
    E_SER_SERVER_IS_RUNNING                                           = 4005,//???????????
    E_SER_SERVER_NOT_RUN                                              = 4006,//??????????
    E_SER_GET_CLIENT_ERROR                                            = 4007,//????Client????
    E_SER_LOGIN_ERROR                                                 = 4008,//?????????
    E_SER_NOT_CONNECTED                                               = 4009,//????
    E_SER_NO_SUCH_USER                                                = 4010,//????????
    E_SER_NOTE_ERROR                                                  = 4011,//??????
    E_SER_CREATE_THREAD_ERROR                                         = 4012,//??????
    E_SER_STRUCT2XML_ERROR                                            = 4013,//STRUCT???XML??
    E_SER_XML2STRUCT_ERROR                                            = 4014,//XML???STRUCT??
    E_SER_XML_FORMAT_ERROR                                            = 4015,//????XML????
    E_SER_PROTOCOL_IS_TOOLARGE                                        = 4016,//?????????????
    E_SER_PEOTOCOL_FORMAT_ERROR                                       = 4017,//????????
    E_SER_GET_PEOTOCOL_TIMEOUT                                        = 4018,//??????
    E_SER_GET_SESSIONID_ERROR                                         = 4019,//??SESSIONID??
    E_SER_SESSIONID_CHECK_ERROR                                       = 4020,//??SESSIONID???
    E_SER_SESSIONID_NOTFIND                                           = 4021,//???SESSIONID???
    E_SER_CONNECTED_NOTFIND                                           = 4022,//??????????
    E_SER_SEND_DATA_ERROR                                             = 4023,//??????
    E_SER_RECV_DATA_ERROR                                             = 4024,//??????
	E_SER_NVR_TIMEOUT												  = 4025,//NVR????


    /*Business*/

	//??
    E_BUS_PARAM_ERROR                                                 = 5001,//????????????
	E_BUS_MAINTAINING                                                 = 5002,//?????????
    E_BUS_UNKNOWN_CMD                                                 = 5003,//?????????
	E_BUS_QUEUE_OVERFLOW                                              = 5004,//??????????
	E_BUS_NO_ENCODER                                                  = 5005,//?????????????
	E_BUS_NO_LIVESTREAM											      = 5006,//????????????
	E_BUS_NO_CAMERA													  = 5007,//???????
	E_BUS_PTZ_BYOTHER 												  = 5008,//???????????
	E_BUS_NO_PRIVILEGE												  = 5009,//????
	E_BUS_LOW_PRIORITY												  = 5010,//???
	E_BUS_NO_DECODER 												  = 5011,//????????????
	E_BUS_NO_MONITOR												  = 5012,//??????
	E_BUS_MACRO_NOT_RUN                                               = 5013,//?????
	E_BUS_NOT_SUPPORTED                                               = 5014, //??????
	E_BUS_NO_NVR                                               		  = 5015,//???NVR
	E_BUS_ERROR_NODEIP												  = 5016,//?????IP??, ????????
	E_BUS_NOT_SAME_NVR												  = 5017, //???????????NVR?
	E_BUS_NO_MACRO													  = 5018,//????
	E_BUS_NO_PORT													  = 5019,//?????????????
	E_BUS_CAMERA_LOCKED 											  = 5020,//??????
	E_BUS_MONITOR_LOCKED 											  = 5021, //??????
	E_BUS_PRESET_ALREADYEXIST										  = 5022, //????????
	E_BUS_NO_PRESET													  = 5023, //???????
	E_BUS_OTHERS_PRESET											      = 5024, // ?????????
	E_BUS_MANAGED_PRESET											  = 5025, // ??????????
	E_BUS_NO_MEDIASERVER											  = 5026, //????????
	E_BUS_MEDIASERVER_DISABLE										  = 5027, //???????
	E_BUS_AUTORECORD												  = 5028, //??????, ??????
	E_BUS_NO_VIDEOTAG												  = 5029, //???????
	E_BUS_NO_FOREVERTAG												  = 5030, //?????????
	E_BUS_NOT_ENOUGH_LIC											  = 5031, //???????
	

	//?????
	E_BUS_DB_OP_ERROR                                                 = 5101,//???????????
	E_BUS_LOAD_RESOURCE_EXCEPT                                        = 5106,//??????????????
	E_BUS_DB_QUERY_FAIL												  = 5107,//??????
	E_BUS_DB_INSERT_FAIL											  = 5108,//??????
	E_BUS_DB_UPDATE_FAIL											  = 5109,//??????
	E_BUS_DB_DELETE_FAIL											  = 5110,//??????
	E_BUS_DB_CREATE_EMPTY_FAIL										  = 5111,//???????
	E_DATAPOOL_GETTBLRECORDFILE_FAILED                                = 5112,//??????????
	E_BUS_SCAN_RUN												      = 5113,//???????
	E_BUS_SCAN_NOTEXISTED										 	  = 5114,//??????
	E_BUS_SCAN_EMPTYCAMERA											  = 5115,//?????????
	//???
	E_BUS_PROC_EXCEPT                                                 = 5201,//??????????
	E_BUS_PROC_FAIL                                                   = 5202,//????????
	E_BUS_UNFIND_REQ                                                  = 5203,//???????Req??


	//????
    E_DISK_PARAM_ERROR                                                 = 5301,//????????????


	//????
	E_KBD_LOAD_KEYBOARD_DATA_FAILED                                   = 5401,//????????
	E_KBD_LOAD_DRIVER_FAILED									      = 5402,//????????
	E_KBD_NOT_SUPPORTED												  = 5403, //????

	//???
	E_MACRO_IS_RUNNING												  = 5501, //?????
	E_MACRO_IS_NOTRUNNING											  = 5502, //??????
	
    /*UserManager*/
    E_USR_PARAM_ERROR                                                 = 8001,//????
    E_USR_OP_DB_ERROR                                                 = 8002,//???????
    E_USR_NO_SUCH_USER                                                = 8003,//????
    E_USR_PASSWD_ERROR                                                = 8004,//??????
    E_USR_NOT_LOGIN                                                   = 8005,//??????
    E_USR_ALREADY_LOGINED                                             = 8006,//??????
    E_USR_NO_SUCH_RESOURCE                                            = 8007,//????
    E_USR_TOO_MANY_USER												  = 8008,//??????

    /*TrunkManager*/
    E_TRK_NO_PATH                                                	  = 9001,//???
    E_TRK_DISPATCHTRUNKERROR                                          = 9002,//??????
    E_TRK_NOUSETRUNKROLE                                       		  = 9003,//???????
    E_TRK_NOOCCUPYROLE                                    			  = 9004,//???????
	E_TRK_LOWPRIVELEGE												  = 9005,//???
	E_TRK_DISABLE													  = 9006,// ??????


	/*DeviceManager*/
	E_DEV_NO_CAMERASETTING                                            = 10001,//???????????

	
	
	
	//VOD??
	ERR_VOD_INVALID_VODFILE 				= 1015001,		//??VOD????
	ERR_VOD_INVALID_VODFILE_FORMAT			= 1015002,		//???VOD????
	ERR_VOD_FILE_NOT_OPENED 				= 1015003,		//VOD???????
	ERR_VOD_FILE_ALREADY_PLAYING			= 1015004,		//VOD??????????
	ERR_VOD_SET_POSITION_ERROR				= 1015005,		//??VOD????
	ERR_VOD_BUILD_FILE_INDEX_ERROR			= 1015006,		//??VOD ????
	ERR_VOD_OPEN_FILE_INDEX_ERROR			= 1015007,		//??VOD??????
	ERR_VOD_INVALID_VOD_INDEX_FILE			= 1015008,		//???VOD????
	ERR_VOD_NO_SENDING_TARGET				= 1015009,		//?????VOD????
	ERR_VOD_NO_SERVER						= 1015010,		//???????????							
	ERR_VOD_OPT_FILE						= 1015011,		//??????
	ERR_VOD_INVALID_ARG 					= 1015012,		//??????
	ERR_VOD_INVALID_VIDEOFILE				= 1015013,		//?????????
	ERR_VOD_NO_SUCH_POSTITION				= 1015014,		//????????
	ERR_VOD_INVALID_INDEXFILE				= 1015015,		//?????????
	ERR_VOD_NOT_INITIALIZED 				= 1015016,		//????VOD??????
	ERR_VOD_UNEXPECTED						= 1015017,		//??????
	ERR_DOWNLOAD_NO_SERVER					= 1015018,		//???????????
	ERR_VOD_DOWNLOAD_NOFILELIST 			= 1015019,		//??????
	ERR_STORAGE_NO_SERVER					= 1015020,		//???????????
	ERR_VOD_INVALID_SESSION					= 1015021,      //????????
	
	
	//DownLoad??
	ERR_DNLD_OPT_FILE						= 1021001,		//??????
	ERR_DNLD_INVAILD_ARG					= 1021002,		//??????
	ERR_DNLD_MALLOC 						= 1021003,		//??????
	ERR_DNLD_DEST_NOT_ASSIGNED				= 1021004,		//???????
	ERR_DNLD_NOT_STOPPED					= 1021005,		//????
	ERR_DNLD_NOT_INITIALIZED				= 1021006,		//????VOD??????
	ERR_DNLD_TOO_MANY_FILES 				= 1021007,		//??????
	ERR_DNLD_SOCKET_CLOSED					= 1021008,		//SOCKET??
	ERR_DNLD_UNEXPECTED 					= 1021009,		//????
	ERR_DNLD_NOFILE                         = 1021010,      //?????


	
	//??????
	ERR_CMP_SEND_SWITCH_CODE				= 1013001,		//??????
	ERR_CMP_SEND_PTZ_CODE					= 2013001,		//PTZ????
	ERR_CMP_NO_COMMPORT 					= 3013001,		//??????
	ERR_CMP_SEND_CODE						= 3013002,		//??????



	ERR_RCK_NO_RACK 						= 3008001,		//????
	ERR_RCK_NO_INPUTPORT					= 3008002,		//??????
	ERR_RCK_NO_OUTPUTPORT					= 3008003,		//??????
	ERR_RACK_NO_ALARMIOPORT 				= 3008004,		//?????IO??
	ERR_RACK_NO_DRIVER						= 3008005,		//?????
	ERR_RACK_NO_TEXTOVERLAY 				= 3008006,		//???????
	ERR_RACK_NO_TEXT						= 3008007,		//?????
	ERR_RACK_NO_MULTIPLEX					= 3008008,		//???????
	
	ERR_RACK_INVAILD_PARAM					= 4008001,		//??????
	ERR_RACK_SUPPORT_PTZTYPE				= 4008002,		//????PTZ??
	ERR_RACK_SUPPORT_MULTIPLEXMOD			= 4008003,		//??????????
	ERR_RACK_OPERATE_FAILED 				= 4008004,		//??????
	ERR_RACK_TEXTOVERLAY_OPERATE_FAILED 	= 4008005,		//?????????
	ERR_RACK_MULTIPLEX_OPERATE_FAILED		= 4008006,		//?????????
	ERR_RACK_NO_ASSOCIATE_COMMPORT			= 4008007,		//?????????

	
	//CODEC??
	ERR_BCD_NO_SUCH_CODEC					= 3016001,		//??????
	ERR_BCD_OPERATE_CODEC_FAILED			= 3016002,		//????????
	ERR_BCD_UNSUPPORTED_CODEC_PARAM	    	= 3016003,      //????????
	ERR_BCD_INVALID_CHANNEL					= 3016004, 	    //?????????
	ERR_BCD_UNSUPPORTED_CODEC_RESOLUTION	= 3016005,      //???????
	ERR_BCD_UNSUPPORTED_CODEC_STREAMNO		= 3016006,      //??????
	ERR_BCD_UNSUPPORTED_CODEC_FORMAT		= 3016007,      //????????
	ERR_BCD_SET_ENCOD_PARAM					= 3016008,      //????????
	ERR_BCD_LOGIN_CODEC						= 3016009,      //???????
	


	PARAM_OPEN_DIR_FALSE = 5021001,
	PARAM_NO_MATCH_DRIVER_PROTOCOL_FILE = 5021002,
	PARAM_XML_FILE_NAME_ERRO = 5021003,
	PARAM_CALL_BACK_ADDR_NULL = 5021004,
	PARAM_CALL_OBJ_NULL = 5021005,
	PARAM_ACTION_TYPE_VALUE_OVERFLOW_ARRAY = 5021006,
	PARAM_SETTED_VARI_DATA_IS_INCORRECT = 5021007,
	
	CREATE_THREAD_ERRO = 5021008,
	ALLOT_MEMORY_ERRO = 5021009,
	WARNIG_CALLBACK_RET_FALSE = 5021010,
	XMLDRIVERERRO_FORMAT = 5021020,
	XMLDRIVERERRO_DEBUG = 5021050,

	XMLFORMAT_DRIVERNODE_HAVE_NULL_CHILDNODE = 5021021,
	XMLFORMAT_ACTIONTYPENODE_HAVE_NULL_CHILDNODE = 5021022,
	XMLFORMAT_ACTIONTYPENODE_IS_NULL_STRING = 5021023,
	XMLFORMAT_VALUENODE_VALUE_IS_NULL_STRING = 5021024,
	XMLFORMAT_BYTENODE_HAVE_NULL_CHILDNODE = 5021025,
	XMLFORMAT_STRNODE_HAVE_NULL_CHILDNODE = 5021026,
	XMLFORMAT_CAN_NOT_FIND_AUTHORNODE_UNDER_DRIVERINFONODE = 5021027,
	XMLFORMAT_CAN_NOT_FIND_VERSIONNODE_UNDER_DRIVERINFONODE = 5021028,
	XMLFORMAT_CAN_NOT_FIND_UPDATETIMENODE_UNDER_DRIVERINFONODE = 5021029,
	XMLFORMAT_CAN_NOT_FIND_DRIVERINFONODE_IN_XMLFILE = 5021030,
	XMLFORMAT_CAN_NOT_FIND_DRIVERNODE_IN_XMLFILE = 5021031,
	XMLFORMAT_CAN_NOT_FIND_SIZENODE_UNDER_ACTIONTYPENODE = 5021032,
	XMLFORMAT_CAN_NOT_FIND_LOOPNODE_UNDER_ACTIONTYPENODE = 5021033,
	XMLFORMAT_CAN_NOT_FIND_INTERVALNODE_UNDER_ACTIONTYPENODE = 5021034,
	XMLFORMAT_CAN_NOT_FIND_TYPE_OR_FORMAT_NODE_UNDER_STRNODE = 5021035,
	XMLFORMAT_CAN_NOT_FIND_ACTNODE_IN_XMLFILE = 5021036,
	XMLFORMAT_CAN_NOT_FIND_TYPE_OR_FORMAT_NODE_UNDER_BYTENODE = 5021037,
	XMLFORMAT_CAN_NOT_FIND_VALUENODE_UNDER_BYTENODE = 5021038,
	XMLFORMAT_CAN_NOT_FIND_VALUENODE_UNDER_STRNODE = 5021039, 
	XMLFORMAT_ACTIONSZIE_UNEQUAL_BYTENODE_NUM = 5021040,
	XMLFORMAT_INCORRECT_DATA_VALUE_SET = 5021041,
	XMLFORMAT_ACT_CODE_SIZE_NEGATIVE = 5021042, 
	XMLFORMAT_SIZE_VALUE_IS_INCORRECT = 5021043,
	XMLFORMAT_INTERVAL_VALUE_IS_INCORRECT = 5021044,
     
	DEBUG_PARSE_VALUE_FALSE = 5021051,
	DEBUG_PARSE_BYTES_FALSE = 5021052,
	DEBUG_PARSE_SINGLE_BYTE_FALSE = 5021053,
	DEBUG_PARSE_STRS_FALSE = 5021054,
	DEBUG_SetCallBack_SET_CALL_BACK_FALSE = 5021055,
	DEBUG_CAN_NOT_FIND_CSESSIO_WITH_SESSIONID_IN_VECTOR = 5021056,
	DEBUG_CAN_NOT_GET_ACT_ATTR_INFO = 5021057,
	DEBUG_GetStrCodeRecord_ERRO_STRFORMAT_VARI_TYPE = 5021058,
	DEBUG_NEW_CSESSION_ERRO = 5021059,
	DEBUG_GetSpeedRange_SPEED_VARI_ATTR_INFO_HAVE_WRONG = 5021060,
	DEBUG_UpdateDriverCodeRecode_ARRAYFLOW_ERRO = 5021061, 
	DEBUG_GetByteCodeRecord_BYTE_RECORDS_PTR_NULL = 5021062,
	DEBUG_GetStrCodeRecord_RESSIZE_ERRO = 5021063,
	DEBUG_GetByteCodeRecord_JY_ERRO = 5021064,
	DEBUG_GetByteCodeRecord_JY_FORMAT_ERRO = 5021065,
	DEBUG_GetByteCodeRecord_UNKNOW_BYTETYPE = 5021066,
	DEBUG_GetDriverCodeRecord_LOOPTYPE_NOT_MATCH = 5021067,
	DEBUG_GetDriverCodeRecord_UNKNOW_ELEFAT_TYPE = 5021068,
	DEBUG_BeToStop_SPEEDPARAM_WRONG = 5021069,
	DEBUG_DoAction_GET_WRONG_SPEEDVALUE = 5021070,
	DEBUG_GetStrCodeRecord_CURSTRRECORDS_PTR_NULL = 5021071,
	DEBUG_ParseValue_UNKNOW_NODE_FORMAT = 5021072,
	DEBUG_CALCULATE_OFFSET_FALSE = 5021073,
	DEBUG_PARSE_DOC_FALSE = 5021074,
	DEBUG_GET_NODE_PTR_FALSE = 5021075

};

#endif


