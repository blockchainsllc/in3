"""
Implementation of a https transport module for Incubed requests
"""
import urllib.parse
import urllib.request

from in3.exception import TransportException
from in3.libin3.transport import In3Request, In3Response


def https_transport(in3_request: In3Request, in3_response: In3Response):
    """
    Transports each request coming from libin3 to the in3 network and and reports the answer back
    Args:
        in3_request (In3Request): request sent by the In3 Client Core to the In3 Network
        in3_response (In3Response): response to be dispatched to the In3 Client Core
    Returns:
        exit_status (int): Always zero for signaling libin3 the function executed OK.
    """

    for i in range(0, in3_request.urls_len()):
        try:
            request_params = {
                'url': str(in3_request.url_at(i), 'utf8'),
                'method': 'POST',
                'data': in3_request.payload(),
                'headers': {'Content-type': 'application/json', 'Accept': 'application/json'},
            }
            request = urllib.request.Request(**request_params)
            timeout = in3_request.timeout() if in3_request.timeout() > 0 else 180000
            with urllib.request.urlopen(request, timeout=timeout) as response:
                if not response.status == 200:
                    raise TransportException('Request failed with status: {}'.format(str(response.status)))
                msg = response.read()
                in3_response.success(i, msg)
        except Exception as err:
            in3_response.failure(i, str(err).encode('utf8'))
    return 0
