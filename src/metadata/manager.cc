#include <vector>

#include "common/bitmap.h"
#include "metadata/inode.h"
#include "metadata/manager.h"

namespace chfs
{

/**
 * Transform a raw inode ID that index the table to a logic inode ID (and vice
 * verse) This prevents the inode ID with 0 to mix up with the invalid one
 */
#define RAW_2_LOGIC(i) (i + 1)
#define LOGIC_2_RAW(i) (i - 1)

  InodeManager::InodeManager(std::shared_ptr<BlockManager> bm,
                             u64 max_inode_supported)
      : bm(bm)
  {
    // 1. calculate the number of bitmap blocks for the inodes
    auto inode_bits_per_block = bm->block_size() * KBitsPerByte;
    auto blocks_needed = max_inode_supported / inode_bits_per_block;

    // we align the bitmap to simplify bitmap calculations
    if (blocks_needed * inode_bits_per_block < max_inode_supported)
    {
      blocks_needed += 1;
    }
    this->n_bitmap_blocks = blocks_needed;

    // we may enlarge the max inode supported
    this->max_inode_supported = blocks_needed * KBitsPerByte * bm->block_size();

    // 2. initialize the inode table
    auto inode_per_block = bm->block_size() / sizeof(block_id_t);
    auto table_blocks = this->max_inode_supported / inode_per_block;
    if (table_blocks * inode_per_block < this->max_inode_supported)
    {
      table_blocks += 1;
    }
    this->n_table_blocks = table_blocks;

    // 3. clear the bitmap blocks and table blocks
    for (u64 i = 0; i < this->n_table_blocks; ++i)
    {
      bm->zero_block(i + 1); // 1: the super block
    }

    for (u64 i = 0; i < this->n_bitmap_blocks; ++i)
    {
      bm->zero_block(i + 1 + this->n_table_blocks);
    }
  }

  auto InodeManager::create_from_block_manager(std::shared_ptr<BlockManager> bm,
                                               u64 max_inode_supported)
      -> ChfsResult<InodeManager>
  {
    auto inode_bits_per_block = bm->block_size() * KBitsPerByte;
    auto n_bitmap_blocks = max_inode_supported / inode_bits_per_block;

    CHFS_VERIFY(n_bitmap_blocks * inode_bits_per_block == max_inode_supported,
                "Wrong max_inode_supported");

    auto inode_per_block = bm->block_size() / sizeof(block_id_t);
    auto table_blocks = max_inode_supported / inode_per_block;
    if (table_blocks * inode_per_block < max_inode_supported)
    {
      table_blocks += 1;
    }

    InodeManager res = {bm, max_inode_supported, table_blocks, n_bitmap_blocks};
    return ChfsResult<InodeManager>(res);
  }

  // { Your code here }
  auto InodeManager::allocate_inode(InodeType type, block_id_t bid)
      -> ChfsResult<inode_id_t>
  {
    auto iter_res = BlockIterator::create(this->bm.get(), 1 + n_table_blocks,
                                          1 + n_table_blocks + n_bitmap_blocks);
    if (iter_res.is_err())
    {
      return ChfsResult<inode_id_t>(iter_res.unwrap_error());
    }

    inode_id_t count = 0;

    // Find an available inode ID.
    for (auto iter = iter_res.unwrap(); iter.has_next();
         iter.next(bm->block_size()).unwrap(), count++)
    {
      auto data = iter.unsafe_get_value_ptr<u8>();
      auto bitmap = Bitmap(data, bm->block_size());
      auto free_idx = bitmap.find_first_free();

      if (free_idx)
      {
        // If there is an available inode ID.

        // Setup the bitmap.
        bitmap.set(free_idx.value());
        auto res = iter.flush_cur_block();
        if (res.is_err())
        {
          return ChfsResult<inode_id_t>(res.unwrap_error());
        }

        // 使用blockAllocator分配一个空闲block，存放inode数据
        // BlockAllocator blockAllocator(bm, 1 + this->n_table_blocks + this->n_bitmap_blocks, false);
        // auto allocateResult = blockAllocator.allocate();
        // if (allocateResult.is_err())
        // {
        //   return ChfsResult<inode_id_t>(allocateResult.unwrap_error());
        // }
        // auto blockIndex = allocateResult.unwrap();

        // 初始化inode状态，并将其写入到分配到的block
        // std::cout << "my code begin" << std::endl;
        Inode newInode(type, this->bm->block_size());
        // std::cout << "init new Inode, then flushed to " << bid << std::endl;
        std::vector<u8> buffer(bm->block_size());
        newInode.flush_to_buffer(buffer.data());
        bm->write_block(bid, buffer.data());

        // 将block number写入对应inode table对应entry
        // std::cout << "set table" << std::endl;
        set_table(free_idx.value(), bid);

        return ChfsResult<inode_id_t>(RAW_2_LOGIC(free_idx.value()));
        // TODO:
        // 1. Initialize the inode with the given type.
        // 2. Setup the inode table.
        // 3. Return the id of the allocated inode.
        //    You may have to use the `RAW_2_LOGIC` macro
        //    to get the result inode id.
        // UNIMPLEMENTED();
      }
    }

    return ChfsResult<inode_id_t>(ErrorType::OUT_OF_RESOURCE);
  }

  // { Your code here }
  auto InodeManager::set_table(inode_id_t idx, block_id_t bid) -> ChfsNullResult
  {
    auto inode_per_blocks = bm->block_size() / sizeof(inode_id_t);
    auto inode_table_blocks = idx / inode_per_blocks;
    auto inode_table_index = idx % inode_per_blocks;
    std::vector<u8> buffer(bm->block_size());
    bm->write_partial_block(1 + inode_table_blocks, reinterpret_cast<u8 *>(&bid), inode_table_index * sizeof(block_id_t), sizeof(block_id_t));
    // TODO: Implement this function.
    // Fill `bid` into the inode table entry
    // whose index is `idx`.
    // UNIMPLEMENTED();

    return KNullOk;
  }

  // { Your code here }
  auto InodeManager::get(inode_id_t id) -> ChfsResult<block_id_t>
  {
    if (id >= max_inode_supported - 1)
    {
      return ChfsResult<block_id_t>(ErrorType::INVALID_ARG);
    }

    auto raw_inode_id = LOGIC_2_RAW(id);
    // auto total_bits_per_blocks = bm->block_size() / KBitsPerByte;
    // auto inode_blocks = raw_inode_id / total_bits_per_blocks;
    // auto inode_index = raw_inode_id % total_bits_per_blocks;

    std::vector<u8> buffer(bm->block_size());
    // bm->read_block(1 + n_table_blocks + inode_blocks, buffer.data());
    // Bitmap inode_bitmap(buffer.data(), bm->block_size());
    // if (!inode_bitmap.check(inode_index))
    // {
    //   return ChfsResult<block_id_t>(ErrorType::INVALID_ARG);
    // }

    block_id_t bid;
    auto inode_per_blocks = bm->block_size() / sizeof(inode_id_t);
    auto inode_table_blocks = raw_inode_id / inode_per_blocks;
    auto inode_table_index = raw_inode_id % inode_per_blocks;
    bm->read_block(1 + inode_table_blocks, buffer.data());
    bid = *reinterpret_cast<block_id_t *>(buffer.data() + inode_table_index * sizeof(block_id_t));
    // std::cout << "get inode table blocks: " << inode_table_blocks << "\n get inode table index: " << inode_table_index << std::endl;
    //  TODO: Implement this function.
    //  Get the block id of inode whose id is `id`
    //  from the inode table. You may have to use
    //  the macro `LOGIC_2_RAW` to get the inode
    //  table index.
    //  UNIMPLEMENTED();

    return ChfsResult<block_id_t>(bid);
  }

  auto InodeManager::free_inode_cnt() const -> ChfsResult<u64>
  {
    auto iter_res = BlockIterator::create(this->bm.get(), 1 + n_table_blocks,
                                          1 + n_table_blocks + n_bitmap_blocks);

    if (iter_res.is_err())
    {
      return ChfsResult<u64>(iter_res.unwrap_error());
    }

    u64 count = 0;
    for (auto iter = iter_res.unwrap(); iter.has_next();)
    {
      auto data = iter.unsafe_get_value_ptr<u8>();
      auto bitmap = Bitmap(data, bm->block_size());

      count += bitmap.count_zeros();

      auto iter_res = iter.next(bm->block_size());
      if (iter_res.is_err())
      {
        return ChfsResult<u64>(iter_res.unwrap_error());
      }
    }
    return ChfsResult<u64>(count);
  }

  auto InodeManager::get_attr(inode_id_t id) -> ChfsResult<FileAttr>
  {
    std::vector<u8> buffer(bm->block_size());
    auto res = this->read_inode(id, buffer);
    if (res.is_err())
    {
      return ChfsResult<FileAttr>(res.unwrap_error());
    }
    Inode *inode_p = reinterpret_cast<Inode *>(buffer.data());
    return ChfsResult<FileAttr>(inode_p->inner_attr);
  }

  auto InodeManager::get_type(inode_id_t id) -> ChfsResult<InodeType>
  {
    std::vector<u8> buffer(bm->block_size());
    auto res = this->read_inode(id, buffer);
    if (res.is_err())
    {
      return ChfsResult<InodeType>(res.unwrap_error());
    }
    Inode *inode_p = reinterpret_cast<Inode *>(buffer.data());
    return ChfsResult<InodeType>(inode_p->type);
  }

  auto InodeManager::get_type_attr(inode_id_t id)
      -> ChfsResult<std::pair<InodeType, FileAttr>>
  {
    std::vector<u8> buffer(bm->block_size());
    auto res = this->read_inode(id, buffer);
    if (res.is_err())
    {
      return ChfsResult<std::pair<InodeType, FileAttr>>(res.unwrap_error());
    }
    Inode *inode_p = reinterpret_cast<Inode *>(buffer.data());
    return ChfsResult<std::pair<InodeType, FileAttr>>(
        std::make_pair(inode_p->type, inode_p->inner_attr));
  }

  // Note: the buffer must be as large as block size
  auto InodeManager::read_inode(inode_id_t id, std::vector<u8> &buffer)
      -> ChfsResult<block_id_t>
  {
    if (id >= max_inode_supported - 1)
    {
      return ChfsResult<block_id_t>(ErrorType::INVALID_ARG);
    }

    auto block_id = this->get(id);
    if (block_id.is_err())
    {
      return ChfsResult<block_id_t>(block_id.unwrap_error());
    }

    if (block_id.unwrap() == KInvalidBlockID)
    {
      return ChfsResult<block_id_t>(ErrorType::INVALID_ARG);
    }

    auto res = bm->read_block(block_id.unwrap(), buffer.data());
    if (res.is_err())
    {
      return ChfsResult<block_id_t>(res.unwrap_error());
    }
    return ChfsResult<block_id_t>(block_id.unwrap());
  }

  // {Your code}
  auto InodeManager::free_inode(inode_id_t id) -> ChfsNullResult
  {
    // std::cout << "free_inode: " << id << std::endl;
    //  simple pre-checks
    if (id >= max_inode_supported - 1)
    {
      return ChfsNullResult(ErrorType::INVALID_ARG);
    }

    auto raw_inode_id = LOGIC_2_RAW(id);
    auto total_bits_per_blocks = bm->block_size() * KBitsPerByte;
    auto inode_blocks = raw_inode_id / total_bits_per_blocks;
    auto inode_index = raw_inode_id % total_bits_per_blocks;

    std::vector<u8> buffer(bm->block_size());
    bm->read_block(1 + n_table_blocks + inode_blocks, buffer.data());
    Bitmap inode_bitmap(buffer.data(), bm->block_size());
    // std::cout << "bitmap check" << std::endl;
    if (!inode_bitmap.check(inode_index))
    {
      return ChfsNullResult(ErrorType::INVALID_ARG);
    }
    // std::cout << "bitmap not set " << std::endl;
    inode_bitmap.clear(inode_index);
    bm->write_block(1 + n_table_blocks + inode_blocks, buffer.data());

    auto inode_per_blocks = bm->block_size() / sizeof(inode_id_t);
    auto inode_table_blocks = raw_inode_id / inode_per_blocks;
    auto inode_table_index = raw_inode_id % inode_per_blocks;
    block_id_t flushed_entry_data = KInvalidBlockID;
    bm->write_partial_block(1 + inode_table_blocks, (u8 *)(&flushed_entry_data), inode_table_index * sizeof(block_id_t), sizeof(block_id_t));
    // std::cout << "write inode table blocks: " << inode_table_blocks << "\n write inode table index: " << inode_table_index << std::endl;
    // TODO:
    // 1. Clear the inode table entry.
    //    You may have to use macro `LOGIC_2_RAW`
    //    to get the index of inode table from `id`.
    // 2. Clear the inode bitmap.
    // UNIMPLEMENTED();

    return KNullOk;
  }

} // namespace chfs