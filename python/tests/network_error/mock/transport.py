"""
Test transport function for `in3.client.Client`.

Collect mocked responses from `http.py`.
"""
import hashlib

from in3.libin3.transport import In3Request, In3Response
from tests.network_error.mock import http


def mock_transport(in3_request: In3Request, in3_response: In3Response):
    """
    Transports each request coming from libin3 to the in3 network and and reports the answer back
    Args:
        in3_request (In3Request): request sent by the In3 Client Core to the In3 Network
        in3_request (In3Response): response to be dispatched to the In3 Client Core
    Returns:
        exit_status (int): Always zero for signaling libin3 the function executed OK.
    """

    for i in range(0, in3_request.urls_len()):
        try:
            http_request = {
                'url': in3_request.url_at(i),
                'data': in3_request.payload(),
                'headers': {'Content-type': 'application/json'},
                'timeout': in3_request.timeout()
            }
            uuid = hashlib.sha1()
            uuid.update(http_request['url'])
            uuid.update(http_request['data'])
            response = http.data[http_request['url']][uuid]
            in3_response.success(i, response)
        except Exception as err:
            err_bytes = str(err).encode('utf8')
            in3_response.failure(i, err_bytes)
    return 0
