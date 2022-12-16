#include <stdint.h>
typedef struct {
  uint32_t fn;
  char*    signature;
} abi_fn_t;

const abi_fn_t abi_known_functions[] = {
    {.signature = "addOwnerWithThreshold(address owner,uint256 threshold)", .fn = 0xd582f13},
    {.signature = "approveHash(bytes32 hashToApprove)", .fn = 0xd4d9bdcd},
    {.signature = "changeThreshold(uint256 threshold)", .fn = 0x694e80c3},
    {.signature = "disableModule(address prevModule,address module)", .fn = 0xe009cfde},
    {.signature = "enableModule(address module)", .fn = 0x610b5925},
    {.signature = "execTransaction(address to,uint256 value,bytes data,uint8 operation,uint256 safeTxGas,uint256 baseGas,uint256 gasPrice,address gasToken,address refundReceiver,bytes signatures)", .fn = 0x6a761202},
    {.signature = "execTransactionFromModule(address to,uint256 value,bytes data,uint8 operation)", .fn = 0x468721a7},
    {.signature = "execTransactionFromModuleReturnData(address to,uint256 value,bytes data,uint8 operation)", .fn = 0x5229073f},
    {.signature = "removeOwner(address prevOwner,address owner,uint256 threshold)", .fn = 0xf8dc5dd9},
    {.signature = "requiredTxGas(address to,uint256 value,bytes data,uint8 operation)", .fn = 0xc4ca3a9c},
    {.signature = "setFallbackHandler(address handler)", .fn = 0xf08a0323},
    {.signature = "setGuard(address guard)", .fn = 0xe19a9dd9},
    {.signature = "setup(address[] owners,uint256 threshold,address to,bytes data,address fallbackHandler,address paymentToken,uint256 payment,address paymentReceiver)", .fn = 0xb63e800d},
    {.signature = "simulateAndRevert(address targetContract,bytes calldataPayload)", .fn = 0xb4faba09},
    {.signature = "swapOwner(address prevOwner,address oldOwner,address newOwner)", .fn = 0xe318b52b},
    {.signature = "addRecoveryKey(address recoveryKey,uint256 interval)", .fn = 0x1e80e5b},
    {.signature = "cancelChallengeAsRecoveryKey(address owner)", .fn = 0x91737042},
    {.signature = "cancelChallengeByChallengedOwner()", .fn = 0x2442b898},
    {.signature = "cancellChallengeByChallengedOwner(address challengedOwner,bytes signature)", .fn = 0xcd3874e8},
    {.signature = "challenge(address owner,address nextOwner)", .fn = 0x5e8b9976},
    {.signature = "changeChallengeSettings(uint256 newReductionValue,uint256 newMinChallengeTime)", .fn = 0xd3019c54},
    {.signature = "changeInterval(uint256 newInterval)", .fn = 0x63c75607},
    {.signature = "joinChallenge(address challengedOwner,bytes32 challengeIdentifier)", .fn = 0xde80046b},
    {.signature = "joinChallengeModule(address challengedOwner,bytes32 challenger,uint256 newInterval)", .fn = 0xfb7d2790},
    {.signature = "removeRecoveryKey(address prevKey,address recoveryKey)", .fn = 0x4f23c90c},
    {.signature = "setup(address[] recoveryKeys,uint256[] intervals)", .fn = 0x28814f03},
    {.signature = "transferOwnershipAfterChallenge(address prevOwner,address ownerToTransfer)", .fn = 0x942cc08d},
    {.signature = "approve(address spender,uint256 amount)", .fn = 0x95ea7b3},
    {.signature = "transfer(address to,uint256 amount)", .fn = 0xa9059cbb},
    {.signature = "transferFrom(address from,address to,uint256 amount)", .fn = 0x23b872dd},
    {.signature = "approve(address to,uint256 tokenId)", .fn = 0x95ea7b3},
    {.signature = "safeTransferFrom(address from,address to,uint256 tokenId)", .fn = 0x42842e0e},
    {.signature = "safeTransferFrom(address from,address to,uint256 tokenId,bytes data)", .fn = 0xb88d4fde},
    {.signature = "setApprovalForAll(address operator,bool approved)", .fn = 0xa22cb465},
    {.signature = "transferFrom(address from,address to,uint256 tokenId)", .fn = 0x23b872dd},
    {.signature = "safeBatchTransferFrom(address from,address to,uint256[] ids,uint256[] amounts,bytes data)", .fn = 0x2eb2c2d6},
    {.signature = "safeTransferFrom(address from,address to,uint256 id,uint256 amount,bytes data)", .fn = 0xf242432a},
    {.signature = "authorizeOperator(address operator)", .fn = 0x959b8c3f},
    {.signature = "burn(uint256 amount,bytes data)", .fn = 0xfe9d9303},
    {.signature = "operatorBurn(address account,uint256 amount,bytes data,bytes operatorData)", .fn = 0xfc673c4f},
    {.signature = "operatorSend(address sender,address recipient,uint256 amount,bytes data,bytes operatorData)", .fn = 0x62ad1b83},
    {.signature = "revokeOperator(address operator)", .fn = 0xfad8b32a},
    {.signature = "send(address recipient,uint256 amount,bytes data)", .fn = 0x9bd9bbc6},
    {.signature = "addChallengeRoleAndOtherRoles(address challenger,uint256 roles,uint256 interval)", .fn = 0x2625f953},
    {.signature = "addChallengerAndOtherRolesWithThreshold(address owner,uint32 threshold,uint256 roles,uint256 interval)", .fn = 0x2d25e12b},
    {.signature = "addOwnerWithThreshold(address owner,uint32 threshold,uint256 roles)", .fn = 0x4b3b1a19},
    {.signature = "addRoles(address owner,uint256 roles)", .fn = 0x2dcaf1cf},
    {.signature = "calculateRolesValueFromRoleEnum(uint8 role)", .fn = 0xde1b178c},
    {.signature = "changeChallengeSettings(address owner,uint256 newReductionValue,uint256 newMinChallengeTime)", .fn = 0xfd02602f},
    {.signature = "changeInterval(address challenger,uint256 newInterval)", .fn = 0xe1bc8896},
    {.signature = "changeMasterCopy(address masterCopy)", .fn = 0x7de7edef},
    {.signature = "changeThreshold(uint32 threshold)", .fn = 0x2cc9c465},
    {.signature = "checkForValidSignatures(bytes32 dataHash,bytes32 originalDataHash,bytes signatures)", .fn = 0xfa8978a8},
    {.signature = "handlePaymentModule(uint256 gasUsed,uint256 baseGas,uint256 gasPrice,address gasToken,address refundReceiver)", .fn = 0xc162d342},
    {.signature = "initiateChallengeModule(address sender,address owner,address nextOwner)", .fn = 0x211646e},
    {.signature = "isValidSignature(bytes32 dataHash,bytes signature)", .fn = 0x1626ba7e},
    {.signature = "removeChallengeFromListModule(address owner)", .fn = 0x6510bb05},
    {.signature = "removeOwner(address prevOwner,address owner,uint32 threshold)", .fn = 0xfcd3b7b1},
    {.signature = "removeRoles(address owner,uint256 roles)", .fn = 0x3f6bc027},
    {.signature = "setup(address[] owners,uint256[] rolesList,uint32 threshold,address to,bytes data,address fallbackHandler,address paymentToken,uint256 payment,address paymentReceiver)", .fn = 0x43bb5014},
    {.signature = "signMessage(bytes32 dataHash)", .fn = 0xa08519b5},
    {.signature = "transferTokenModule(address token,address receiver,uint256 amount)", .fn = 0x85a9126e},
    {.signature = "calculateCreateProxyWithNonceAddress(address mastercopy,bytes initializer,uint256 saltNonce)", .fn = 0x2500510e},
    {.signature = "createProxy(address masterCopy,bytes data)", .fn = 0x61b69abd},
    {.signature = "createProxyWithCallback(address mastercopy,bytes initializer,uint256 saltNonce,address callback)", .fn = 0xd18af54d},
    {.signature = "createProxyWithNonce(address mastercopy,bytes initializer,uint256 saltNonce)", .fn = 0x1688f0b9},
    {.signature = "createProxyWithNonceForZksync(address mastercopy,bytes initializer,bytes20 pubKeyHash)", .fn = 0xf9fcfd5d},
    {.signature = "proxyCreationCode()", .fn = 0x53e5d935},
    {.signature = "proxyRuntimeCode()", .fn = 0xaddacc0f},
    {.signature = "multiSend(bytes transactions)", .fn = 0x8d80ff0a},
    {.signature = "addLimit(address token,(uint256 period,uint256 currentPeriod,uint256 spent,uint256 limit))", .fn = 0x312902cd},
    {.signature = "addToAllowList(address account)", .fn = 0x31f59102},
    {.signature = "changeLimit(address token,uint256 index,uint256 newLimit,uint256 limitAmount,uint256 oldPeriod)", .fn = 0xaff6df54},
    {.signature = "deleteLimit(address token,uint256 index,uint256 limit,uint256 period)", .fn = 0x884e888},
    {.signature = "executeAllowed(address to,uint256 value,bytes data)", .fn = 0x64ed6dfa},
    {.signature = "executeAllowed(address to,uint256 value,bytes data,uint8 operation,uint256 safeTxGas,uint256 baseGas,uint256 gasPrice,address gasToken,address refundReceiver,bytes signature)", .fn = 0xd1d3bcde},
    {.signature = "executeLimit(address token,address to,uint256 amount)", .fn = 0x5bdfc4d},
    {.signature = "executeLimit(address token,address to,uint256 amount,uint8 operation,uint256 safeTxGas,uint256 baseGas,uint256 gasPrice,address gasToken,address refundReceiver,bytes signature)", .fn = 0xf19e407f},
    {.signature = "heartbeat()", .fn = 0x3defb962},
    {.signature = "heartbeat(address challengedOwner,bytes signature)", .fn = 0x7ab4790f},
    {.signature = "removeChallenge(address owner)", .fn = 0x3be61616},
    {.signature = "removeFromAllowlist(address prevAllowed,address accountToRemove)", .fn = 0x47a6a971},
    {.signature = "setup(address token,(uint256 period,uint256 currentPeriod,uint256 spent,uint256 limit)[],address[] accountsToAllow)", .fn = 0x4b6d2a80},
    {.signature = "createAndAddModules(address proxyFactory,bytes data)", .fn = 0x60df7f58},
    {.signature = "mint(address to,uint256 id,uint256 amount,bytes32 contentHash,string tokenUri,bool unique,bytes data,(address receipient,uint8 percent)[])", .fn = 0xfb33ccb4},
    {.signature = "pause()", .fn = 0x8456cb59},
    {.signature = "recoverAddress(bytes32 hash,uint8 v,bytes32 r,bytes32 s)", .fn = 0x8428cf83},
    {.signature = "redeem(address redeemer,(uint256 tokenId,uint256 minPrice,bytes32 contentHash,string tokenUri,bool unique,address signer,address[] benefectors,uint8[] percents,uint8 v,bytes32 r,bytes32 s))", .fn = 0x95056d3b},
    {.signature = "renounceOwnership()", .fn = 0x715018a6},
    {.signature = "setBaseURI(string newuri)", .fn = 0x55f804b3},
    {.signature = "setURI(string newuri)", .fn = 0x2fe5305},
    {.signature = "transferOwnership(address newOwner)", .fn = 0xf2fde38b},
    {.signature = "unpause()", .fn = 0x3f4ba83a}};