from in3.exception import ClientException
from in3.eth.factory import EthObjectFactory
from in3.libin3.runtime import In3Runtime
from in3.eth.model import RawTransaction, TransactionReceipt
from in3.libin3.enum import EthMethods


class EthAccountApi:
    """
    Manages wallets and smart-contracts
    """

    def __init__(self, runtime: In3Runtime, factory: EthObjectFactory):
        self._runtime = runtime
        self._factory = factory

    def sign(self, address: str, data: str or RawTransaction) -> str:
        """
        Use ECDSA to sign a message.
        Args:
            address (str): Ethereum address of the wallet that will sign the message.
            data (str): Data to be signed, EITHER a hash string or a Transaction.
        Returns:
            signed_message (str): ECDSA calculated r, s, and parity v, concatenated. v = 27 + (r % 2)
        """
        # in3_ret_t eth_sign(void* ctx, d_signature_type_t type, bytes_t message, bytes_t account, uint8_t* dst)
        #           eth_sign(ctx,                   SIGN_EC_RAW,         *data,           bytes(NULL, 0),   sig))
        #   SIGN_EC_RAW  = 0, /**< sign the data directly */ SIGN_EC_HASH = 1, /**< hash and sign the data */
        address = self._factory.get_account(address)
        signature = bytearray()
        if isinstance(data, str):
            self._factory.get_hash(data)
            return_code = self._runtime.call(
                EthMethods.SIGN, 0, address, data, signature)
        elif isinstance(data, RawTransaction):
            raise NotImplementedError
            # TODO: This uses only the tx.data. Check with Shoaib. Should be a json of all filled in params
            # TODO: Move to in3_signData
            # return_code = self._runtime.call(EthMethods.SIGN, 1, address, data, None, signature)
        else:
            raise ValueError
        if return_code and return_code == 65:
            return str(signature)
        else:
            raise ClientException('Unknown error trying to sign the provided data.')

    def send_transaction(self, transaction: RawTransaction) -> str:
        """
        Signs and sends the assigned transaction. Requires the 'key' value to be set in ClientConfig.
        Transactions change the state of an account, just the balance, or additionally, the storage and the code.
        Every transaction has a cost, gas, paid in Wei. The transaction gas is calculated over estimated gas times the
        gas cost, plus an additional miner fee, if the sender wants to be sure that the transaction will be mined in the
        latest block.
        Args:
            transaction: All information needed to perform a transaction. Minimum is from, to and value.
            Client will add the other required fields, gas and chaindId.
        Returns:
            tx_hash: Transaction hash, used to get the receipt and check if the transaction was mined.
        """
        assert isinstance(transaction, RawTransaction)
        return self._runtime.call(EthMethods.SEND_TRANSACTION, transaction)

    def send_raw_transaction(self, transaction: RawTransaction) -> str:
        """
        Sends a signed and encoded transaction.
        Args:
            transaction: All information needed to perform a transaction. Minimum is from, to and value.
            Client will add the other required fields, gas and chaindId.
        Returns:
            tx_hash: Transaction hash, used to get the receipt and check if the transaction was mined.
        """
        assert isinstance(transaction, RawTransaction)
        return self._runtime.call(EthMethods.SEND_RAW_TRANSACTION, transaction)

    # TODO: Create Receipt domain object
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

    def estimate_gas(self, transaction: RawTransaction) -> int:
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
