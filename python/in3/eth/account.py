import random

from in3.eth.factory import EthObjectFactory
from in3.eth.model import NewTransaction, Account
from in3.exception import PrivateKeyNotFoundException
from in3.libin3.enum import EthMethods, BlockAt, In3Methods
from in3.libin3.runtime import In3Runtime


class EthAccountApi:
    """
    Manages accounts and smart-contracts
    """

    def __init__(self, runtime: In3Runtime, factory: EthObjectFactory):
        self._runtime = runtime
        self._factory = factory

    def _create_account(self, secret: str) -> Account:
        if not secret.startswith('0x') or not len(secret) == 66:
            TypeError('Secret must be a 256 bit hexadecimal.')
        address = self._runtime.call(In3Methods.PK_2_ADDRESS, secret)
        account = self._factory.get_account(address, int(secret, 16))
        return account

    def create(self, qrng=False) -> Account:
        """
        Creates a new Ethereum account and saves it in the wallet.
        Args:
            qrng (bool): True uses a Quantum Random Number Generator api for generating the private key.
        Returns:
            account (Account): Newly created Ethereum account.
        """
        if qrng:
            raise NotImplementedError
        secret = hex(random.getrandbits(256))
        return self._create_account(secret)

    def recover(self, secret: str) -> Account:
        """
        Recovers an account from a secret.
        Args:
            secret (str): Account private key in hexadecimal string
        Returns:
            account (Account): Recovered Ethereum account.
        """
        if not secret or not len(secret) == 66:
            raise PrivateKeyNotFoundException('Please provide account secret.')
        return self._create_account(secret)

    def parse_mnemonics(self, mnemonics: str) -> str:
        """
        Recovers an account secret from mnemonics phrase
        Args:
            mnemonics (str): BIP39 mnemonics phrase.
        Returns:
            secret (str): Account secret. Use `recover_account` to create a new account with this secret.
        """
        raise NotImplementedError

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

    def balance(self, address: str, at_block: int or str = str(BlockAt.LATEST)) -> int:
        """
        Returns the balance of the account of given address.
        Args:
            address (str): address to check for balance
            at_block (int or str):  block number IN3BlockNumber  or EnumBlockStatus
        Returns:
            balance (int): integer of the current balance in wei.
        """
        account = self._factory.get_account(address)
        if isinstance(at_block, int):
            at_block = hex(at_block)
        return self._factory.get_integer(self._runtime.call(EthMethods.BALANCE, account.address, at_block))

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
        if not sender.secret or not len(hex(sender.secret)) == 66:
            raise AssertionError('To send a transaction, the sender\'s secret must be known by the application. \
            To send a pre-signed transaction use `send_raw_transaction` instead.')
        transaction.From = sender.address
        self._runtime.set_signer_account(sender.secret)
        return self._runtime.call(EthMethods.SEND_TRANSACTION, transaction.serialize())

    def send_raw_transaction(self, signed_transaction: str) -> str:
        """
        Sends a signed and encoded transaction.
        Args:
            signed_transaction: Signed keccak hash of the serialized transaction
            Client will add the other required fields, gas and chaindId.
        Returns:
            tx_hash (hex): Transaction hash, used to get the receipt and check if the transaction was mined.
        """
        return self._runtime.call(EthMethods.SEND_RAW_TRANSACTION, signed_transaction)

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

    def transaction_count(self, address: str, at_block: int or str = str(BlockAt.LATEST)) -> int:
        """
        Number of transactions mined from this address. Used to set transaction nonce.
        Nonce is a value that will make a transaction fail in case it is different from (transaction count + 1).
        It exists to mitigate replay attacks.
        Args:
            address (str): Ethereum account address
            at_block (int):  Block number
        Returns:
            tx_count (int): Number of transactions mined from this address.
        """
        account = self._factory.get_account(address)
        if isinstance(at_block, int):
            at_block = hex(at_block)
        tx_count = self._runtime.call(EthMethods.TRANSACTION_COUNT, account.address, at_block)
        return self._factory.get_integer(tx_count)

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
