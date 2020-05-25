"""
Test transport function for `in3.client.Client`.

Collect mocked responses from `http.py`.
"""
import ctypes as c
import hashlib

from in3.libin3.lib_loader import libin3
from in3.transport import In3Request
from tests.network_error.mock import http


@c.CFUNCTYPE(c.c_int, c.POINTER(In3Request))
def mock_transport(in3_request: In3Request):
    """
    Transports each request coming from libin3 to the in3 network and and reports the answer back
    Args:
        in3_request (In3Request): request sent by the In3 Client Core to the In3 Network
        positive: True for positive tests, False for negative (must fail nicely) tests
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
            uuid = hashlib.sha1()
            uuid.update(http_request['url'])
            uuid.update(http_request['data'])
            response = http.data[http_request['url']][uuid]
            libin3.in3_req_add_response(in3_request.results, i, False, response, len(response))
        except Exception as err:
            err_bytes = str(err).encode('utf8')
            libin3.in3_req_add_response(in3_request.results, i, True, err_bytes, len(str(err)))
    return 0
