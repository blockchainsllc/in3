IF (USE_SEGGER_RTT)
add_library(segger_rtt_o OBJECT
            SEGGER_RTT.c
            SEGGER_RTT.h
            SEGGER_RTT_Conf.h
            SEGGER_RTT_printf.c
        )
add_library(segger_rtt STATIC $<TARGET_OBJECTS:segger_rtt_o>)
ENDIF(USE_SEGGER_RTT)
