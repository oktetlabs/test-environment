
///gsoap cwmp  service port:      http://localhost:80/
///gsoap cwmp  service transport: http://schemas.xmlsoap.org/soap/http
//gsoap cwmp  service location:      https://localhost/

struct SOAP_ENV__Header {
  mustUnderstand char *cwmp__ID;
  mustUnderstand int  *cwmp__HoldRequests;
};

int __cwmp__GetRPCMethods(_cwmp__GetRPCMethods *cwmp__GetRPCMethods, _cwmp__GetRPCMethodsResponse *cwmp__GetRPCMethodsResponse);
int __cwmp__GetParamterValues(_cwmp__GetParameterValues *cwmp__GetParameterValues, _cwmp__GetParameterValuesResponse *cwmp__GetParameterValuesResponse);
int __cwmp__GetParamterNames(_cwmp__GetParameterNames *cwmp__GetParameterNames, _cwmp__GetParameterNamesResponse *cwmp__GetParameterNamesResponse);
