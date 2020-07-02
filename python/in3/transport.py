import http.client
from http import HTTPStatus

from in3.libin3.transport import In3Request, In3Response


def http_transport(in3_request: In3Request, in3_response: In3Response):
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
            endpoint = {
                'host': in3_request.url_at(i),
                'timeout': in3_request.timeout()
            }
            request = {
                'body': in3_request.payload(),
                'headers': {'Content-type': 'application/json', 'Accept': 'application/json'},
            }
            endpoint = http.client.HTTPConnection(**endpoint)
            endpoint.request(**request)
            response = endpoint.getresponse()
            if not response.status == HTTPStatus.OK:
                raise http.client.HTTPException('Request failed with status: {}'.format(str(response.status)))
            if 'error' in response.text:
                in3_response.failure(i, response.content)
            else:
                in3_response.success(i, response.content)
        except http.client.HTTPException as err:
            in3_response.failure(i, str(err).encode('utf8'))
        except Exception as err:
            in3_response.failure(i, str(err).encode('utf8'))
    return 0
