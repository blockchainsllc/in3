OPTION(ETH "if true, the ethereum verifiers will be build" ON)
IF (ETH)
    add_subdirectory(eth1/nano)
    add_subdirectory(eth1/basic)
    add_subdirectory(eth1/full)
    add_subdirectory(eth1/evm)
ENDIF(ETH)

OPTION(BTC "if true, the bitcoin verifiers will be build" ON)
IF (BTC)
    add_subdirectory(btc)
ENDIF(BTC)

