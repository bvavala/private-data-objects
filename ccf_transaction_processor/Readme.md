<!---
Licensed under Creative Commons Attribution 4.0 International License
https://creativecommons.org/licenses/by/4.0/
--->

## Microsoft CCF based PDO Transaction Processor

This folder contains software for PDO transaction processor (TP) based on Microsoft's CCF blockchain.
The software is located under transaction_processor/. The folder CCF/ points to CCF tag 0.7.1 which is
included as a submodule under the PDO repo. The TP software is written and tested for CCF tag 0.7.1.
Compatability with other CCF versions is not guaranteed.

The TP must viewed as a CCF application. Documentation for building and deploying CCF applications
can be found at https://microsoft.github.io/CCF/. The CCF legder that stores the PDO TP registries is
encrypted, and is accessible only within CCF encalves. Currently PDO/CCF combination is supported
only under the virtual enclave mode for both PDO and CCF. (set env variable SGX_MODE=SIM for PDO &
set cmake flag TARGET=virtual for ccf). Support for HW mode for both PDO and CCF will be added soon. Further,
the current implementation of PDO TP hasn't been tested under multi-threaded CCF enclaves.(CCF 0.7.1 offers initial support for multi-threading). It is recommended that the application is deployed with single worker thread per enclave. Please see https://microsoft.github.io/CCF/developers/threading.html for instructions.

CCF uses mutually authenticated TLS channels for transactions. Given that in PDO client authentication is implemented within the transaction processor itself, we do not utilize the client authentication feature provided by CCF. Once CCF is deployed, CCF's network certificate (networkcert.pem) and one set of user keys (userccf_cert.pem & userccf_privk.pem) must be made available to all PDO processes that want to submit a CCF transaction. In this case, every PDO process behaves as though it is a CCF user corresponding to the private key userccf_privk.pem.
These keys must be stored under $PDO_LEDGER_KEY_ROOT as part of PDO deployment.

It may be noted that PDO also supports TP based on the Hyperledger Sawtooth blockchain.
As far as PDO is concerned, CCF based TP is functionally identical to the Sawtooth based
TP (except for one additional feature described below). A key difference beween the two ledgers
is the fact that while the ledger in CCF is encrypted as noted above, the ledger is Sawtooth is stored in plain text.
Even though the no part of conract state gets stored in the ledger in both CCF & Sawtooth, encrypting the
ledger as in CCF helps to hide transaction patterns that are otherwise visible in Sawtooth ledger. Detailed documentation about Sawtooth based TP can be found at $PDO_SRC/sawtooth/docs. The schema for JSON payloads used to submit CCF transactions can be found at [${PDO_SRC}/python/pdo/submitter/ccf/docs/](../python/pdo/submitter/ccf/docs/ccf_payload_schema.json). For additional references to documentation about PDO, including transaction processor protocols, see [${PDO_SRC}/README.md](../README.md)

A feature of the CCF based TP that is not supported by Sawtooth based TP is the fact that
responses to read-transactions include a payload signature, where the signature is generated by the CCF enclave
serving the read request. The (signing key, verifying key) pair is created within the TP,
and is globally persisted among all CCF enclaves. The verifying_key can be obtained from CCF using
the "get_ledger_verifying_key" transaction. This feature may be used by PDO clients to establish offline verifiable proof of transaction commits as desired by the PDO smart contract application. Note that for trust purposes, it is recommended that any entity that uses the above verifying_key gets it directly from the CCF service using the
"get_ledger_verifying_key" transaction.

We note that the very first invocation of "get_ledger_verifying_key" rpc
after the service is started is used to create the key pair, which will then be globally committed.
The system admin who deploys the CCF ledger is expected to invoke the "get_ledger_verifying_key"
rpc after deployment to generate these keys. Further, the admin must ensure that this key pair is globally committed
before making the ledger available for PDO usage. This can checked by issuing the rpc again & ensuring that a verifying key gets returned. See https://microsoft.github.io/CCF/users/issue_commands.html for instructions on
how an rpc can be issued via command line.
