import ctypes as c
import glob
import json

from pathlib import Path

from in3 import ClientConfig
from in3.libin3.lib_loader import libin3
from in3.transport import In3Request
from tests import client_mock

mock = {}
mock.update(client_mock.data)
# mock_files = glob.glob(str(Path(Path(__file__).parent, '*.mock.json')))
# [mock.update(_dict) for _dict in [json.load(open(mock_file)) for mock_file in mock_files]]

mock_client_config = ClientConfig(node_list_auto_update=False, request_retries=1, node_signature_consensus=2,
                                  node_signatures=2)


@c.CFUNCTYPE(c.c_int, c.POINTER(In3Request))
def mock_transport(in3_request: In3Request):
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
            request_data = json.loads(http_request['data'])[0]
            request_method = request_data['method']
            response = mock[request_method][http_request['url']]
            libin3.in3_req_add_response(in3_request.results, i, False, response, len(response))
        except Exception as err:
            err_bytes = str(err).encode('utf8')
            libin3.in3_req_add_response(in3_request.results, i, True, err_bytes, len(str(err)))
    return 0
