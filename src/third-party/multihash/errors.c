#include "errors.h"

const char *mh_error_string(int code) {
	switch (code) {
		case MH_E_NO_ERROR:
			return "no error";
		case MH_E_UNKNOWN_CODE:
			return "unknown multihash code";
		case MH_E_TOO_SHORT:
			return "multihash too short. must be > 2 bytes";
		case MH_E_TOO_LONG:
			return "multihash too long. must be < 129 bytes";
		case MH_E_VARINT_NOT_SUPPORTED:
			return "c-multihash does not yet support"
				" varint encoding";
		case MH_E_DIGSET_TOO_LONG:
			return "c-multihash does not support digsets"
				" longer than 127 bytes yet";
		default:
			return "unknown error code";
	}
}
