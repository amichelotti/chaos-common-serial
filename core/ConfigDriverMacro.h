/**
 * Bring parameters expressed in json as plain C parameters
 * in CHAOS environment the json is wrapped into CDataWrapper
 * in other json-cpp library is used
 * */
#ifndef DRIVER_CONFIG_MACRO
#define DRIVER_CONFIG_MACRO

#ifdef CHAOS
#include <chaos/common/data/CDataWrapper.h>
#include <chaos/common/exception/CException.h>
typedef boost::shared_ptr<chaos::common::data::CDataWrapper> json_t;

#define GET_int32_t(x,d) x->getInt32Value(d)

#define GET_double(x,d) x->getDoubleValue(d)
#define GET_float(x,d) static_cast<float>(x->getDoubleValue(d))

#define GET_int64_t(x,d) x->getInt64Value(d)
#define GET_string(x,d) x->getStringValue(d)
#define GET_bool(x,d) x->getBoolValue(d)
#define GET_json_t(x,d) json_t(x->getCSDataValue(d))

#define GET_PARAMETER_TREE(_config,_param) \
		json_t _param;\
		if( (_config)->hasKey(#_param)){\
			_param=GET_json_t(_config,# _param);\
		}\
		if(! (_config)->hasKey(#_param)){\
			throw chaos::CException(-1," missing required parameter '" # _param "'",__PRETTY_FUNCTION__);\
		} else


#define GET_PARAMETER(_config,_param,_ctype,_required)\
	_ctype _param;\
	if(_config->hasKey(#_param)){\
		_param=GET_ ## _ctype(_config,# _param);\
		LDBG_<<"["<< __PRETTY_FUNCTION__ <<"] Getting parameter '"<< # _param <<"'="<<_param;\
	} else if(_required){\
		LERR_<<"["<< __PRETTY_FUNCTION__ <<"] missing required parameter '" # _param "'";\
		throw chaos::CException(-1,"missing required parameter '" # _param "'",__PRETTY_FUNCTION__);\
	}
#define GET_PARAMETER_DEFAULT(_config,_param,_ctype,_default)\
	_ctype _param=_default;\
	if(_config->hasKey(#_param)){\
		_param=GET_ ## _ctype(_config,# _param);\
	}
#define GET_PARAMETER_DO(_config,_param,_ctype,_required)\
		_ctype _param;\
		if(_config->hasKey(#_param)){\
			LDBG_<<"["<< __PRETTY_FUNCTION__ <<"] Getting parameter '"<< # _param <<"'";\
			_param=GET_ ## _ctype(_config,# _param);\
		} else if(_required){LERR_<< "["<< __PRETTY_FUNCTION__ <<"] missing required parameter '" # _param "'";throw chaos::CException(-1,"missing required parameter '" # _param "'",__PRETTY_FUNCTION__);}\
		if(_config->hasKey(#_param))
#endif
#endif
