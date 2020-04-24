import ctypes as c

import requests

from in3.libin3.lib_loader import libin3


class In3Request(c.Structure):
    """
    Request sent by the libin3 to the In3 Network, transported over the _http_transport function
    Based on in3/client/.h in3_request_t struct
    Attributes:
        payload (str): the payload to send
        urls ([str]): array of urls
        urls_len (int): number of urls
        results (str): the responses
        timeout (int): the timeout 0= no timeout
        times (int): measured times (in ms) which will be used for ajusting the weights
    """
    _fields_ = [("payload", c.POINTER(c.c_char)),
                ("urls", c.POINTER(c.POINTER(c.c_char))),
                ("urls_len", c.c_int),
                ("results", c.c_void_p),
                ("timeout", c.c_uint32),
                ("times", c.c_uint32)]


def _transport_report_success(in3_request: In3Request, i: int, response: requests.Response):
    libin3.in3_req_add_response(in3_request.results, i, False, response.content, len(response.content))


def _transport_report_failure(in3_request: In3Request, i: int, msg: bytes):
    libin3.in3_req_add_response(in3_request.results, i, True, msg, len(msg))


@c.CFUNCTYPE(c.c_int, c.POINTER(In3Request))
def http_transport(in3_request: In3Request):
    """
    Transports each request coming from libin3 to the in3 network and and reports the answer back
    Args:
        in3_request (In3Request): request sent by the In3 Client Core to the In3 Network
    Returns:
        exit_status (int): Always zero for signaling libin3 the function executed OK.
    """
    in3_request = in3_request.contents

    for i in range(0, in3_request.urls_len):
        try:
            http_request = {
                'url': c.string_at(in3_request.urls[i]),
                'data': c.string_at(in3_request.payload),
                'headers': {'Content-type': 'application/json'},
                'timeout': in3_request.timeout
            }
            response = requests.post(**http_request)
            response.raise_for_status()
            if 'error' in response.text:
                _transport_report_failure(in3_request, i, response.content)
            else:
                _transport_report_success(in3_request, i, response)
        except requests.exceptions.RequestException as err:
            _transport_report_failure(in3_request, i, str(err).encode('utf8'))
        except requests.exceptions.SSLError as err:
            _transport_report_failure(in3_request, i, str(err).encode('utf8'))
        except Exception as err:
            _transport_report_failure(in3_request, i, str(err).encode('utf8'))
    return 0
