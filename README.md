# Distributed File System (DFS) - A GFS-like Architecture

This project implements a distributed file system inspired by Google File System (GFS). It is structured into four labs, each building upon the previous one to achieve a scalable, fault-tolerant, and efficient distributed file system with additional support for distributed computation.

---

## Project Overview

The project is divided into four labs:

1. **Lab 1**: Build an Inode-based file system.
2. **Lab 2**: Extend the Inode-based file system to a distributed file system.
3. **Lab 3**: Integrate the Raft consensus algorithm for strong consistency.
4. **Lab 4**: Implement a MapReduce-like distributed computation framework on top of the distributed file system.

---

## Architecture

### Lab 1: Inode-based File System
- Implements a basic file system using **Inodes** to manage file metadata and data blocks.
- Provides foundational functionalities such as file creation, deletion, reading, and writing.

### Lab 2: Distributed File System
The distributed file system consists of three main components:
1. **Client**:
   - Requests file metadata (e.g., chunk IDs) from the **Metadata Server**.
   - Retrieves file content directly from the **Chunk Servers**.
2. **Metadata Server**:
   - Stores file metadata, including file-to-chunk mappings.
   - Ensures atomicity of operations using **Two-Phase Locking (2PL)**.
3. **Chunk Servers**:
   - Store file chunks and ensure data consistency through versioning.
   - Use **Write-Ahead Logging (WAL)** for crash recovery and durability.
   - Trigger **checkpoints** to persist chunks to disk when the log reaches a threshold, preventing unbounded log growth.

### Lab 3: Integration of Raft Consensus Protocol
- **Leader Election**: Ensures a single leader manages the metadata server cluster.
- **Log Replication**: Replicates operations across the metadata server cluster for fault tolerance.
- **Safety**: Guarantees strong consistency and avoids split-brain scenarios.
- **Checkpointing**: Removes old logs to prevent unbounded growth while maintaining system state.

### Lab 4: Distributed Computation with MapReduce
- **Mapper**: Processes input data and emits key-value pairs.
- **Reducer**: Aggregates key-value pairs to produce final results.
- Implements a **word count** task as an example of distributed computation.

---

## Key Features
1. **Scalability**:
   - Distributes file storage across multiple chunk servers.
   - Supports large files by splitting them into chunks.
2. **Fault Tolerance**:
   - Uses Raft for metadata server fault tolerance.
   - Implements WAL and checkpointing for crash recovery.
3. **Consistency**:
   - Ensures chunk versioning to avoid stale or inconsistent data.
   - Uses Two-Phase Locking for atomic metadata operations.
4. **Distributed Computation**:
   - Provides a MapReduce-like framework for distributed data processing.

---

## Technologies Used
- **Programming Language**: [Specify the language, e.g., Go, Java, etc.]
- **Consensus Protocol**: Raft
- **Logging and Recovery**: Write-Ahead Logging (WAL)
- **Distributed Computation**: MapReduce-like framework
- **Concurrency Control**: Two-Phase Locking (2PL)

---


Good luck to you in this distributed file system project! 🚀
