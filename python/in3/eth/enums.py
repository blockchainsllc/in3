def _get_enum_options(cls):
    return [
        cls().__getattribute__(attr)
        for attr in dir(cls)
        if not callable(cls().__getattribute__(attr)) and not attr.startswith(u"_")
    ]


class Chain:
    MAINNET = "mainnet"
    KOVAN = "kovan"
    GOERLI = "goerli"
    EWC = "ewc"

    @staticmethod
    def options():
        return _get_enum_options(Chain)
