import random

from in3.eth.factory import EthObjectFactory
from in3.eth.model import Account
from in3.libin3.enum import In3Methods
from in3.libin3.runtime import In3Runtime


class WalletApi:
    """
    Ethereum accounts holder and manager.
    """

    def __init__(self, runtime: In3Runtime):
        self._runtime = runtime
        self._factory = EthObjectFactory(runtime)
        # TODO: Persistence
        self._accounts: dict = {}

    def new_account(self, name: str, qrng=False) -> Account:
        """
        Creates a new Ethereum account and saves it in the wallet.
        Args:
            name (str): Account identifier to use with `get`. i.e. get('my_wallet`)
            qrng (bool): True uses a quantum random number generator api for generating the private key.
        Returns:
            account (Account): Newly created Ethereum account.
        """
        if qrng:
            raise NotImplementedError
        secret = hex(random.getrandbits(256))
        return self._create_account(name, secret)

    def get(self, name: str) -> Account or None:
        """
        Returns account in case it exists. Doesnt fail for easier handling w/out try catch. i.e: if get('my_wallet'): do
        Args:
            name (str): Account identifier to use with `get`. i.e. get('my_wallet`)
        Returns:
            account (Account): Selected Ethereum account.
        """
        if name not in self._accounts.keys():
            return None
        return self._accounts[name]

    def delete(self, name: str) -> bool:
        """
        Deletes an account in case it exists.
        Args:
            name (str): Account identifier.
        Returns:
            success (bool): True if account exists and was deleted.
        """
        if name not in self._accounts.keys():
            return False
        del self._accounts[name]
        return True

    def recover_account(self, name: str, secret: str) -> Account:
        """
        Recovers an account from a secret.
        Args:
            name (str): Account identifier to use with `get`. i.e. get('my_wallet`)
            secret (str): Account private key in hexadecimal string
        Returns:
            account (Account): Recovered Ethereum account.
        """
        return self._create_account(name, secret)

    def parse_mnemonics(self, mnemonics: str) -> str:
        """
        Recovers an account secret from mnemonics phrase
        Args:
            mnemonics (str): BIP39 mnemonics phrase.
        Returns:
            secret (str): Account secret. Use `recover_account` to create a new account with this secret.
        """
        raise NotImplementedError

    def _create_account(self, name: str, secret: str) -> Account:
        if not secret.startswith('0x') or not len(secret) == 66:
            TypeError('Secret must be a 256 bit hexadecimal.')
        address = self._runtime.call(In3Methods.PK_2_ADDRESS, secret)
        account = self._factory.get_account(address, int(secret, 16))
        self._accounts[name] = account
        return account
