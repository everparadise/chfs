#include "block/allocator.h"
#include "common/bitmap.h"

namespace chfs
{

  BlockAllocator::BlockAllocator(std::shared_ptr<BlockManager> block_manager)
      : BlockAllocator(std::move(block_manager), 0, true) {}

  // Your implementation
  BlockAllocator::BlockAllocator(std::shared_ptr<BlockManager> block_manager,
                                 usize bitmap_block_id, bool will_initialize)
      : bm(std::move(block_manager)), bitmap_block_id(bitmap_block_id)
  {
    // calculate the total blocks required
    const auto total_bits_per_block = this->bm->block_size() * KBitsPerByte;
    auto total_bitmap_block = this->bm->total_blocks() / total_bits_per_block;
    if (this->bm->total_blocks() % total_bits_per_block != 0)
    {
      total_bitmap_block += 1;
    }

    CHFS_VERIFY(total_bitmap_block > 0, "Need blocks in the manager!");
    CHFS_VERIFY(total_bitmap_block + this->bitmap_block_id <= bm->total_blocks(),
                "not available blocks to store the bitmap");

    this->bitmap_block_cnt = total_bitmap_block;
    if (this->bitmap_block_cnt * total_bits_per_block ==
        this->bm->total_blocks())
    {
      this->last_block_num = total_bits_per_block;
    }
    else
    {
      this->last_block_num = this->bm->total_blocks() % total_bits_per_block;
    }
    CHFS_VERIFY(this->last_block_num <= total_bits_per_block,
                "last block num should be less than total bits per block");

    if (!will_initialize)
    {
      return;
    }

    // zeroing
    for (block_id_t i = 0; i < this->bitmap_block_cnt; i++)
    {
      this->bm->zero_block(i + this->bitmap_block_id);
    }

    block_id_t cur_block_id = this->bitmap_block_id;
    std::vector<u8> buffer(bm->block_size());
    auto bitmap = Bitmap(buffer.data(), bm->block_size());
    bitmap.zeroed();

    // set the blocks of the bitmap block to 1
    for (block_id_t i = 0; i < this->bitmap_block_cnt + this->bitmap_block_id;
         i++)
    {
      // + bitmap_block_id is necessary, since the bitmap block starts with an
      // offset
      auto block_id = i / total_bits_per_block + this->bitmap_block_id;
      auto block_idx = i % total_bits_per_block;

      if (block_id != cur_block_id)
      {
        bm->write_block(cur_block_id, buffer.data());

        cur_block_id = block_id;
        bitmap.zeroed();
      }

      bitmap.set(block_idx);
    }

    bm->write_block(cur_block_id, buffer.data());
  }

  // Fixme: currently we don't consider errors in this implementation
  auto BlockAllocator::free_block_cnt() const -> usize
  {
    usize total_free_blocks = 0;
    std::vector<u8> buffer(bm->block_size());

    for (block_id_t i = 0; i < this->bitmap_block_cnt; i++)
    {
      bm->read_block(i + this->bitmap_block_id, buffer.data()).unwrap();

      usize n_free_blocks = 0;
      if (i == this->bitmap_block_cnt - 1)
      {
        // last one
        // std::cerr <<"last block num: " << this->last_block_num << std::endl;
        n_free_blocks = Bitmap(buffer.data(), bm->block_size())
                            .count_zeros_to_bound(this->last_block_num);
      }
      else
      {
        n_free_blocks = Bitmap(buffer.data(), bm->block_size()).count_zeros();
      }
      // std::cerr << "check free block: " << i << " : " << n_free_blocks
      //           << std::endl;
      total_free_blocks += n_free_blocks;
    }
    return total_free_blocks;
  }

  // Your implementation
  auto BlockAllocator::allocate() -> ChfsResult<block_id_t>
  {
    std::vector<u8> buffer(bm->block_size());

    for (uint i = 0; i < this->bitmap_block_cnt; i++)
    {
      bm->read_block(i + this->bitmap_block_id, buffer.data());

      // The index of the allocated bit inside current bitmap block.
      std::optional<block_id_t> res = std::nullopt;

      if (i == this->bitmap_block_cnt - 1)
      {
        // If current block is the last block of the bitmap.
        // std::cout << "bitmap last_block_numer:" << last_block_num << std::endl;
        Bitmap originBitmap(buffer.data(), this->last_block_num / KBitsPerByte);
        res = originBitmap.find_first_free();
        // TODO: Find the first free bit of current bitmap block
        // and store it in `res`.
        // UNIMPLEMENTED();
      }
      else
      {
        Bitmap originBitmap(buffer.data(), bm->block_size());
        res = originBitmap.find_first_free();
        // UNIMPLEMENTED();
      }

      // If we find one free bit inside current bitmap block.
      if (res.has_value())
      {
        // The block id of the allocated block.
        block_id_t retval = static_cast<block_id_t>(0);

        Bitmap modifiedBitmap(buffer.data(), i == this->bitmap_block_cnt - 1 ? this->last_block_num / KBitsPerByte : bm->block_size());
        modifiedBitmap.set(res.value());

        bm->write_block(i + this->bitmap_block_id, buffer.data());

        const auto total_bits_per_block = static_cast<block_id_t>(this->bm->block_size() * KBitsPerByte);
        retval = res.value() + i * total_bits_per_block;
        // TODO:
        // 1. Set the free bit we found to 1 in the bitmap.
        // 2. Flush the changed bitmap block back to the block manager.
        // 3. Calculate the value of `retval`.
        // UNIMPLEMENTED();
        // std::cout << "allocate: " << retval << std::endl;
        CHFS_ASSERT(retval < this->bm->total_blocks(), "allocate fault");
        return ChfsResult<block_id_t>(retval);
      }
    }
    return ChfsResult<block_id_t>(ErrorType::OUT_OF_RESOURCE);
  }

  // Your implementation
  auto BlockAllocator::deallocate(block_id_t block_id) -> ChfsNullResult
  {
    if (block_id >= this->bm->total_blocks())
    {
      return ChfsNullResult(ErrorType::INVALID_ARG);
    }

    std::vector<u8> buffer(bm->block_size());
    const auto total_bits_per_block = static_cast<block_id_t>(this->bm->block_size() * KBitsPerByte);
    const auto target_bitmap_block_id = block_id / total_bits_per_block;
    const auto target_bitmap_index = block_id % total_bits_per_block;

    bm->read_block(this->bitmap_block_id + target_bitmap_block_id, buffer.data());
    Bitmap modifiedBitmap(buffer.data(), bm->block_size());

    if (!modifiedBitmap.check(target_bitmap_index))
      return ChfsNullResult(ErrorType::INVALID_ARG);
    modifiedBitmap.clear(target_bitmap_index);

    bm->write_block(this->bitmap_block_id + target_bitmap_block_id, buffer.data());
    // TODO: Implement this function.
    // 1. According to `block_id`, zero the bit in the bitmap.
    // 2. Flush the changed bitmap block back to the block manager.
    // 3. Return ChfsNullResult(ErrorType::INVALID_ARG)
    //    if you find `block_id` is invalid (e.g. already freed).
    // UNIMPLEMENTED();

    return KNullOk;
  }

} // namespace chfs
