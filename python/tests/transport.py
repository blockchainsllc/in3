import ctypes as c
import json

from in3 import ClientConfig
from in3.libin3.lib_loader import libin3
from in3.transport import In3Request
from tests import client_mock

mock = {}
mock.update(client_mock.data)

_in3_contract = {'0x1': {'contract': '0xac1b824795e1eb1f6e609fe0da9b9af8beaab60f',
                         'registryId': '0x23d5345c5c13180a8080bd5ddbe7cde64683755dcce6e734d95b7b573845facb',
                         'needsUpdate': False,
                         'avgBlockTime': 15},
                 '0x5': {'contract': '0x5f51e413581dd76759e9eed51e63d14c8d1379c8',
                         'registryId': '0x67c02e5e272f9d6b4a33716614061dd298283f86351079ef903bf0d4410a44ea',
                         'needsUpdate': False,
                         'avgBlockTime': 15},
                 '0x2a': {'contract': '0x4c396dcf50ac396e5fdea18163251699b5fcca25',
                          'registryId': '0x92eb6ad5ed9068a24c1c85276cd7eb11eda1e8c50b17fbaffaf3e8396df4becf',
                          'needsUpdate': False,
                          'avgBlockTime': 6},
                 }

mock_config = ClientConfig(node_list_auto_update=False, request_retries=1,
                           node_signature_consensus=2, node_signatures=2,
                           in3_registry=_in3_contract)


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
