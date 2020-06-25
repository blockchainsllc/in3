import requests

from in3.libin3.runtime import In3Request, In3Response


def http_transport(in3_request: In3Request, in3_response: In3Response):
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
            response = requests.post(**http_request)
            response.raise_for_status()
            if 'error' in response.text:
                in3_response.failure(i, response.content)
            else:
                in3_response.success(i, response.content)
        except requests.exceptions.RequestException as err:
            in3_response.failure(i, str(err).encode('utf8'))
        except requests.exceptions.SSLError as err:
            in3_response.failure(i, str(err).encode('utf8'))
        except Exception as err:
            in3_response.failure(i, str(err).encode('utf8'))
    return 0
