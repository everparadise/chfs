#include "metadata/inode.h"

namespace chfs
{

  auto Inode::begin() -> InodeIterator { return InodeIterator(this, 0); }

  auto Inode::end() -> InodeIterator
  {
    return InodeIterator(this, this->nblocks);
  }

  auto Inode::write_indirect_block(std::shared_ptr<BlockManager> &bm,
                                   std::vector<u8> &buffer) -> ChfsNullResult
  {
    if (this->blocks[this->nblocks - 1] == KInvalidBlockID)
    {
      return ChfsNullResult(ErrorType::INVALID_ARG);
    }
    std::cout << "write indirect block: " << this->blocks[this->nblocks - 1] << "\ndata[0]: " << reinterpret_cast<block_id_t *>(buffer.data())[0] << std::endl;
    return bm->write_block(this->blocks[this->nblocks - 1], buffer.data());
  }

} // namespace chfs