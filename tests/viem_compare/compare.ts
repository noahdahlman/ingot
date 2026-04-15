import { privateKeyToAccount } from "viem/accounts";
import { keccak256, toHex, toBytes, hashMessage, serializeTransaction } from "viem";
import { execFileSync } from "node:child_process";
import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const cppBinary = resolve(__dirname, "../../build/tests/test_cross_validate");

// Same test key as in C++ tests (Hardhat account #0)
const PRIVATE_KEY =
  "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80" as const;

const account = privateKeyToAccount(PRIVATE_KEY);

interface TestVector {
  name: string;
  message: string;
}

const vectors: TestVector[] = [
  { name: "empty", message: "" },
  { name: "hello", message: "hello" },
  { name: "hello_world", message: "hello world" },
  {
    name: "long_message",
    message:
      "The quick brown fox jumps over the lazy dog and then does it again",
  },
  { name: "special_chars", message: "!@#$%^&*()_+-=[]{}|;':\",./<>?" },
  { name: "unicode", message: "ethereum \u2248 \u00e9th\u00e9reum" },
];

let passed = 0;
let failed = 0;

// Get C++ output for all test vectors
const cppOutput = execFileSync(cppBinary, { encoding: "utf-8" }).trim();
const cppLines = cppOutput.split("\n");
const cppResults = new Map<string, { address: string; signature: string }>();

for (const line of cppLines) {
  // Format: name|address|signature
  const [name, address, signature] = line.split("|");
  cppResults.set(name, { address, signature });
}

// Verify address matches
const viemAddress = account.address.toLowerCase();
const cppAddress = cppResults.values().next().value!.address;

console.log(`\n=== Cross-validation: ingot (C++) vs viem (TypeScript) ===\n`);
console.log(`Private key: ${PRIVATE_KEY}`);
console.log(`viem address:  ${viemAddress}`);
console.log(`C++ address:   ${cppAddress}`);

if (viemAddress === cppAddress) {
  console.log(`Address: MATCH\n`);
  passed++;
} else {
  console.log(`Address: MISMATCH\n`);
  failed++;
}

// Compare personal_sign for each test vector
for (const { name, message } of vectors) {
  const viemSig = await account.signMessage({ message });
  const cppResult = cppResults.get(name);

  if (!cppResult) {
    console.log(`[FAIL] ${name}: C++ result not found`);
    failed++;
    continue;
  }

  const cppSig = cppResult.signature;
  const match = viemSig.toLowerCase() === cppSig.toLowerCase();

  if (match) {
    console.log(`[PASS] personal_sign("${name}")`);
    passed++;
  } else {
    console.log(`[FAIL] personal_sign("${name}")`);
    console.log(`  viem: ${viemSig}`);
    console.log(`  C++:  ${cppSig}`);
    failed++;
  }
}

// --- Transaction signing cross-validation ---

console.log(`\n--- Transaction signing ---\n`);

// Parse C++ tx results
const cppTxResults = new Map<string, string>();
for (const line of cppLines) {
  const parts = line.split("|");
  if (parts[0].startsWith("tx_")) {
    cppTxResults.set(parts[0], parts[1]);
  }
}

// tx_simple: 0 value, nonce 0
{
  const viemSig = await account.signTransaction({
    chainId: 1,
    nonce: 0,
    maxPriorityFeePerGas: 1000000000n,
    maxFeePerGas: 20000000000n,
    gas: 21000n,
    to: "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045",
    type: "eip1559",
  });

  const cppTx = cppTxResults.get("tx_simple");
  const match = viemSig.toLowerCase() === ("0x" + cppTx).toLowerCase();
  if (match) {
    console.log(`[PASS] sign_transaction(simple)`);
    passed++;
  } else {
    console.log(`[FAIL] sign_transaction(simple)`);
    console.log(`  viem: ${viemSig}`);
    console.log(`  C++:  0x${cppTx}`);
    failed++;
  }
}

// tx_with_value: 1 ETH, nonce 7
{
  const viemSig = await account.signTransaction({
    chainId: 1,
    nonce: 7,
    maxPriorityFeePerGas: 2000000000n,
    maxFeePerGas: 50000000000n,
    gas: 21000n,
    to: "0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045",
    value: 1000000000000000000n, // 1 ETH
    type: "eip1559",
  });

  const cppTx = cppTxResults.get("tx_with_value");
  const match = viemSig.toLowerCase() === ("0x" + cppTx).toLowerCase();
  if (match) {
    console.log(`[PASS] sign_transaction(with_value)`);
    passed++;
  } else {
    console.log(`[FAIL] sign_transaction(with_value)`);
    console.log(`  viem: ${viemSig}`);
    console.log(`  C++:  0x${cppTx}`);
    failed++;
  }
}

console.log(`\n--- Results: ${passed} passed, ${failed} failed ---`);
if (failed > 0) process.exit(1);
