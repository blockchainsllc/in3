# - Try to find scrypt
# Once done this wil define
# scrypt_FOUND
# scrypt_INCLUDE_DIRS
# scrypt_LIBRARIES

find_path(scrypt_INCLUDE_DIR libscrypt.h)
find_library(scrypt_LIBRARY NAMES scrypt libscrypt)

set(scrypt_LIBRARIES ${scrypt_LIBRARY})
set(scrypt_INCLUDE_DIRS ${scrypt_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(scrypt REQUIRED_VARS scrypt_LIBRARY scrypt_INCLUDE_DIR)

set(scrypt_FOUND ${SCRYPT_FOUND})

if(NOT scrypt_FOUND)
	if(scrypt_FIND_REQUIRED)
		message(SEND_ERROR "Unable to find scrypt crypto library")
	endif(scrypt_FIND_REQUIRED)
endif(NOT scrypt_FOUND)

mark_as_advanced(scrypt_INCLUDE_DIR scrypt_LIBRARY)