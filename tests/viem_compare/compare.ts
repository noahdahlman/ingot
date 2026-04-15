import { privateKeyToAccount } from "viem/accounts";
import { keccak256, toHex, toBytes, hashMessage } from "viem";
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

console.log(`\n--- Results: ${passed} passed, ${failed} failed ---`);
if (failed > 0) process.exit(1);
