from in3.exception import ClientException
from in3.eth.factory import EthObjectFactory
from in3.libin3.runtime import In3Runtime
from in3.eth.model import NewTransaction, TransactionReceipt, Account
from in3.libin3.enum import EthMethods


class EthAccountApi:
    """
    Manages wallets and smart-contracts
    """

    def __init__(self, runtime: In3Runtime, factory: EthObjectFactory):
        self._runtime = runtime
        self._factory = factory

    def sign(self, private_key: str, message: str) -> str:
        """
        Use ECDSA to sign a message.
        Args:
            private_key (str): Must be either an address(20 byte) or an raw private key (32 byte)"}}'
            message (str): Data to be hashed and signed. Dont input hashed data unless you know what you are doing.
        Returns:
            signed_message (str): ECDSA calculated r, s, and parity v, concatenated. v = 27 + (r % 2)
        """
        #   SIGN_EC_RAW  = 0, /**< sign the data directly
        #   SIGN_EC_HASH = 1, /**< hash and sign the data */
        signature_type = 'eth_sign'
        # in3_ret_t in3_sign_data(data, pk, sig_type)
        signature_dict = self._runtime.call(EthMethods.SIGN, message, private_key, signature_type)
        return signature_dict['signature']

    def send_transaction(self, sender: Account, transaction: NewTransaction) -> str:
        """
        Signs and sends the assigned transaction. Requires `account.secret` value set.
        Transactions change the state of an account, just the balance, or additionally, the storage and the code.
        Every transaction has a cost, gas, paid in Wei. The transaction gas is calculated over estimated gas times the
        gas cost, plus an additional miner fee, if the sender wants to be sure that the transaction will be mined in the
        latest block.
        Args:
            sender (Account): Sender Ethereum account. Senders generally pay the gas costs, so they must have enough balance to pay gas + amount sent, if any.
            transaction (NewTransaction): All information needed to perform a transaction. Minimum is to and value. Client will add the other required fields, gas and chaindId.
        Returns:
            tx_hash (hex): Transaction hash, used to get the receipt and check if the transaction was mined.
        """
        assert isinstance(transaction, NewTransaction)
        assert isinstance(sender, Account)
        if not sender.secret or len(sender.secret) < 66:
            raise AssertionError('To send a transaction, the sender\'s secret must be known by the application. \
            To send a pre-signed transaction use `send_raw_transaction` instead.')
        transaction.to = sender.address
        self._runtime.set_signer_account(sender.secret)
        return self._runtime.call(EthMethods.SEND_TRANSACTION, transaction.serialize())

    def send_raw_transaction(self, transaction: NewTransaction) -> str:
        """
        Sends a signed and encoded transaction.
        Args:
            transaction: All information needed to perform a transaction. Minimum is from, to and value.
            Client will add the other required fields, gas and chaindId.
        Returns:
            tx_hash (hex): Transaction hash, used to get the receipt and check if the transaction was mined.
        """
        """
            public void sendRawTransaction() {
            String[][] mockedResponses = {{"eth_sendRawTransaction", "eth_sendRawTransaction_1.json"}};
        
            IN3    in3            = builder.constructClient(mockedResponses);
            String rawTransaction = "0xf8671b8477359400825208943940256b93c4be0b1d5931a6a036608c25706b0c8405f5e100802da0278d2c010a59688fc12a55563d81239b1dc7e3d9c6a535b34600329b0c640ad8a03894daf8d7c25b56caec71b695c5f6b1b6fd903ecfa441b2c4e15fd1c72c54a9";
            String hash           = in3.getEth1API().sendRawTransaction(rawTransaction);
        
            // expect multiple calls here too
            Assertions.assertEquals("0xd55a8b0cf4896ffbbb10b125bf20d89c8006f42cc327a9859c59ac54e439b388", hash);
        """
        """
        SIGN_DATA_NO_LEN=${NOUNCE}${GAS_PRICE}${GAS_LIMIT}${TO}${VALUE}${CODE}${EIP_155}
        LEN IS 2 BYTES
        DATA IS {LEN[0]}{LEN[1]}{SIGN_DATA_NO_LEN}
        THEN KECCAK($SIGN_DATA_NO_LEN)
        BOOM!
        """
        assert isinstance(transaction, NewTransaction)
        return self._runtime.call(EthMethods.SEND_RAW_TRANSACTION, transaction)

    def get_transaction_receipt(self, tx_hash: str) -> TransactionReceipt:
        """
        After a transaction is received the by the client, it returns the transaction hash. With it, it is possible to
        gather the receipt, once a miner has mined and it is part of an acknowledged block. Because how it is possible,
        in distributed systems, that data is asymmetric in different parts of the system, the transaction is only "final"
        once a certain number of blocks was mined after it, and still it can be possible that the transaction is discarded
        after some time. But, in general terms, it is accepted that after 6 to 8 blocks from latest, that it is very
        likely that the transaction will stay in the chain.
        Args:
            tx_hash: Transaction hash.
        Returns:
            tx_receipt: The mined Transaction data including event logs.
        """
        tx_receipt = self._runtime.call(EthMethods.TRANSACTION_RECEIPT, self._factory.get_hash(tx_hash))
        return self._factory.get_tx_receipt(tx_receipt)

    def estimate_gas(self, transaction: NewTransaction) -> int:
        """
        Gas estimation for transaction. Used to fill transaction.gas field. Check RawTransaction docs for more on gas.
        Args:
            transaction: Unsent transaction to be estimated. Important that the fields data or/and value are filled in.
        Returns:
            gas (int): Calculated gas in Wei.
        """
        gas = self._runtime.call(EthMethods.ESTIMATE_TRANSACTION, transaction.serialize())
        return self._factory.get_integer(gas)

    def checksum_address(self, address: str, add_chain_id: bool = True) -> str:
        """
        Will convert an upper or lowercase Ethereum address to a checksum address, that uses case to encode values.
        See [EIP55](https://github.com/ethereum/EIPs/blob/master/EIPS/eip-55.md).
        Args:
            address: Ethereum address string or object.
            add_chain_id (bool): Will append the chain id of the address, for multi-chain support, canonical for Eth.
        Returns:
            checksum_address: EIP-55 compliant, mixed-case address object.
        """
        return self._factory.checksum_address(self._factory.get_account(address).address, add_chain_id)
