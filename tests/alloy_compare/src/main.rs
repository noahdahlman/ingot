use alloy_consensus::{SignableTransaction, TxEip1559};
use alloy_network::TxSignerSync;
use alloy_primitives::{address, hex, Bytes, TxKind, U256};
use alloy_signer::SignerSync;
use alloy_signer_local::PrivateKeySigner;
use std::collections::HashMap;
use std::process::Command;

fn main() {
    // Same test key as C++ and viem tests (Hardhat account #0)
    let signer: PrivateKeySigner =
        "ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80"
            .parse()
            .unwrap();

    // Run C++ binary and collect its output
    let cpp_binary = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "../../build/tests/test_cross_validate".to_string());

    let output = Command::new(&cpp_binary)
        .output()
        .unwrap_or_else(|e| panic!("Failed to run C++ binary at {}: {}", cpp_binary, e));

    assert!(output.status.success(), "C++ binary failed");
    let cpp_output = String::from_utf8(output.stdout).unwrap();

    let mut cpp_sigs: HashMap<String, Vec<String>> = HashMap::new();
    for line in cpp_output.trim().lines() {
        let parts: Vec<&str> = line.split('|').collect();
        let name = parts[0].to_string();
        let fields: Vec<String> = parts[1..].iter().map(|s| s.to_string()).collect();
        cpp_sigs.insert(name, fields);
    }

    let mut passed = 0u32;
    let mut failed = 0u32;

    // Verify address
    let alloy_address = format!("{:?}", signer.address());
    let cpp_address = &cpp_sigs
        .get("hello")
        .expect("C++ output missing 'hello'")[0];

    println!("\n=== Cross-validation: ingot (C++) vs alloy (Rust) ===\n");
    println!("Private key: 0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80");
    println!("alloy address: {}", alloy_address);
    println!("C++ address:   {}", cpp_address);

    if alloy_address.to_lowercase() == cpp_address.to_lowercase() {
        println!("Address: MATCH\n");
        passed += 1;
    } else {
        println!("Address: MISMATCH\n");
        failed += 1;
    }

    // personal_sign test vectors (must match C++ order)
    let vectors = [
        ("empty", ""),
        ("hello", "hello"),
        ("hello_world", "hello world"),
        (
            "long_message",
            "The quick brown fox jumps over the lazy dog and then does it again",
        ),
        ("special_chars", "!@#$%^&*()_+-=[]{}|;':\",./<>?"),
        ("unicode", "ethereum ≈ éthéreum"),
    ];

    for (name, message) in &vectors {
        let sig = signer.sign_message_sync(message.as_bytes()).unwrap();
        let alloy_sig = format!("0x{}", hex::encode(sig.as_bytes()));

        if let Some(cpp_fields) = cpp_sigs.get(*name) {
            let cpp_sig = &cpp_fields[1]; // signature is second field
            if alloy_sig.to_lowercase() == cpp_sig.to_lowercase() {
                println!("[PASS] personal_sign(\"{}\")", name);
                passed += 1;
            } else {
                println!("[FAIL] personal_sign(\"{}\")", name);
                println!("  alloy: {}", alloy_sig);
                println!("  C++:   {}", cpp_sig);
                failed += 1;
            }
        } else {
            println!("[FAIL] {}: C++ result not found", name);
            failed += 1;
        }
    }

    // Transaction signing
    println!("\n--- Transaction signing ---\n");

    // tx_simple: 0 value, nonce 0
    {
        let mut tx = TxEip1559 {
            chain_id: 1,
            nonce: 0,
            max_priority_fee_per_gas: 1_000_000_000,
            max_fee_per_gas: 20_000_000_000,
            gas_limit: 21_000,
            to: TxKind::Call(address!("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045")),
            value: U256::ZERO,
            input: Bytes::new(),
            access_list: Default::default(),
        };

        let sig = signer.sign_transaction_sync(&mut tx).unwrap();
        let signed_tx = tx.into_signed(sig);
        let mut buf = Vec::new();
        buf.push(0x02u8);
        signed_tx.rlp_encode(&mut buf);
        let alloy_hex = hex::encode(&buf);

        if let Some(cpp_fields) = cpp_sigs.get("tx_simple") {
            let cpp_hex = &cpp_fields[0];
            if alloy_hex.to_lowercase() == cpp_hex.to_lowercase() {
                println!("[PASS] sign_transaction(simple)");
                passed += 1;
            } else {
                println!("[FAIL] sign_transaction(simple)");
                println!("  alloy: 0x{}", alloy_hex);
                println!("  C++:   0x{}", cpp_hex);
                failed += 1;
            }
        } else {
            println!("[FAIL] tx_simple: C++ result not found");
            failed += 1;
        }
    }

    // tx_with_value: 1 ETH, nonce 7
    {
        let mut tx = TxEip1559 {
            chain_id: 1,
            nonce: 7,
            max_priority_fee_per_gas: 2_000_000_000,
            max_fee_per_gas: 50_000_000_000,
            gas_limit: 21_000,
            to: TxKind::Call(address!("0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045")),
            value: U256::from(1_000_000_000_000_000_000u64), // 1 ETH
            input: Bytes::new(),
            access_list: Default::default(),
        };

        let sig = signer.sign_transaction_sync(&mut tx).unwrap();
        let signed_tx = tx.into_signed(sig);
        let mut buf = Vec::new();
        buf.push(0x02u8);
        signed_tx.rlp_encode(&mut buf);
        let alloy_hex = hex::encode(&buf);

        if let Some(cpp_fields) = cpp_sigs.get("tx_with_value") {
            let cpp_hex = &cpp_fields[0];
            if alloy_hex.to_lowercase() == cpp_hex.to_lowercase() {
                println!("[PASS] sign_transaction(with_value)");
                passed += 1;
            } else {
                println!("[FAIL] sign_transaction(with_value)");
                println!("  alloy: 0x{}", alloy_hex);
                println!("  C++:   0x{}", cpp_hex);
                failed += 1;
            }
        } else {
            println!("[FAIL] tx_with_value: C++ result not found");
            failed += 1;
        }
    }

    println!("\n--- Results: {} passed, {} failed ---", passed, failed);
    if failed > 0 {
        std::process::exit(1);
    }
}
