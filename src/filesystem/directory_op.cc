#include <algorithm>
#include <sstream>

#include "filesystem/directory_op.h"

namespace chfs
{
  // static std::ostream &operator<<(std::ostream &os, std::vector<u8> &vec)
  // {
  //   os << "[";
  //   for (size_t i = 0; i < vec.size(); ++i)
  //   {
  //     if (i > 0)
  //     {
  //       os << " ";
  //     }
  //     os << static_cast<int>(vec[i]); // 将 u8 转换为 int 以便更好地输出
  //   }
  //   os << "]";
  //   return os;
  // }
  /**
   * Some helper functions
   */
  auto string_to_inode_id(std::string &data) -> inode_id_t
  {
    std::stringstream ss(data);
    inode_id_t inode;
    ss >> inode;
    return inode;
  }

  auto inode_id_to_string(inode_id_t id) -> std::string
  {
    std::stringstream ss;
    ss << id;
    return ss.str();
  }

  // {Your code here}
  auto dir_list_to_string(const std::list<DirectoryEntry> &entries)
      -> std::string
  {
    std::ostringstream oss;
    // usize cnt = 0;
    for (const auto &entry : entries)
    {
      oss << entry.name << ':' << entry.id << "/";
      // if (cnt < entries.size() - 1)
      // {
      //   oss << '/';
      // }
      // cnt += 1;
    }
    return oss.str();
  }

  // {Your code here}
  auto append_to_directory(std::string src, std::string filename, inode_id_t id)
      -> std::string
  {
    std::stringstream oss;
    oss << src;
    oss << filename << ":" << id << "/";
    // TODO: Implement this function.
    //       Append the new directory entry to `src`.
    // UNIMPLEMENTED();

    return oss.str();
  }

  // {Your code here}
  void parse_directory(std::string &src, std::list<DirectoryEntry> &list)
  {
    std::stringstream oss(src);
    std::string pair;
    while (std::getline(oss, pair, '/'))
    {
      std::stringstream pair_oss(std::move(pair));

      std::string name;
      std::getline(pair_oss, name, ':');
      inode_id_t id;
      pair_oss >> id;

      DirectoryEntry entry;
      entry.id = id;
      entry.name = std::move(name);

      list.emplace_back(entry);
    }
    // TODO: Implement this function.
    // UNIMPLEMENTED();
  }

  // {Your code here}
  auto rm_from_directory(std::string src, std::string filename) -> std::string
  {
    std::string pair;
    std::stringstream oss(src);
    std::stringstream iss;
    while (std::getline(oss, pair, '/'))
    {
      std::stringstream pair_oss(std::move(pair));
      std::string name, id;
      std::getline(pair_oss, name, ':');
      if (name == filename)
        continue;
      std::getline(pair_oss, id);
      iss << name << ':' << id << '/';
    }
    // TODO: Implement this function.
    //       Remove the directory entry from `src`.
    // UNIMPLEMENTED();
    return iss.str();
  }

  /**
   * { Your implementation here }
   */
  auto read_directory(FileOperation *fs, inode_id_t id,
                      std::list<DirectoryEntry> &list) -> ChfsNullResult
  {
    std::string content;
    auto dir_res = fs->read_file(id);
    if (dir_res.is_err())
      return ChfsNullResult(dir_res.unwrap_error());

    auto dir_data = dir_res.unwrap();
    content.assign(reinterpret_cast<const char *>(dir_data.data()), dir_data.size());
    // std::cout << "directory content: " << content << std::endl;
    parse_directory(content, list);
    // TODO: Implement this function.
    // UNIMPLEMENTED();

    return KNullOk;
  }

  // {Your code here}
  auto FileOperation::lookup(inode_id_t id, const char *name)
      -> ChfsResult<inode_id_t>
  {
    std::list<DirectoryEntry> list;
    std::string target(name);
    read_directory(this, id, list);
    for (auto &it : list)
    {
      if (it.name == target)
        return it.id;
    }
    // TODO: Implement this function.
    // UNIMPLEMENTED();

    return ChfsResult<inode_id_t>(ErrorType::NotExist);
  }

  // {Your code here}
  auto FileOperation::mk_helper(inode_id_t id, const char *name, InodeType type)
      -> ChfsResult<inode_id_t>
  {
    std::list<DirectoryEntry> list;
    read_directory(this, id, list);
    for (auto &it : list)
    {
      if (it.name == name)
        return ErrorType::AlreadyExist;
    }
    auto alloc_res = this->block_allocator_->allocate();
    if (alloc_res.is_err())
      return alloc_res.unwrap_error();

    auto inode_res = this->inode_manager_->allocate_inode(type, alloc_res.unwrap());
    if (inode_res.is_err())
      return inode_res.unwrap_error();
    // std::cout << "filename: " << std::string(name) << "\n";
    // std::cout << "inode number: " << inode_res.unwrap() << std::endl;
    auto str = dir_list_to_string(list);
    // std::cout << "current dir content: " << str << std::endl;
    auto res = append_to_directory(str, name, inode_res.unwrap());
    this->write_file(id, std::vector<u8>(res.begin(), res.end()));
    // std::cout << "write directory, content: " << res << std::endl;
    //  TODO:
    //  1. Check if `name` already exists in the parent.
    //     If already exist, return ErrorType::AlreadyExist.
    //  2. Create the new inode.
    //  3. Append the new entry to the parent directory.
    //  UNIMPLEMENTED();

    return ChfsResult<inode_id_t>(static_cast<inode_id_t>(inode_res.unwrap()));
  }

  // {Your code here}
  auto FileOperation::unlink(inode_id_t parent, const char *name)
      -> ChfsNullResult
  {
    // std::vector<u8> u8_name(strlen(name));
    // for (int i = 0; i < strlen(name); i++)
    // {
    //   std::cout << *((u8 *)(name + i));
    // }
    // std::cout << std::endl;
    // std::cout << "unlink: " << std::vector<u8>(name) << std::endl;
    /**
     * Remove the file named @name from directory @parent.
     * Free the file's blocks.
     *
     * @return  If the file doesn't exist, indicate error ENOENT.
     * @return  ENOTEMPTY if the deleted file is a directory
     */
    auto look_res = lookup(parent, name);

    // not exist
    if (look_res.is_err())
      return look_res.unwrap_error();
    // std::cout << "unlink: look up finish" << std::endl;
    inode_id_t target_id = look_res.unwrap();
    std::vector<u8> inode(this->block_manager_->block_size());
    auto inode_res = this->inode_manager_->read_inode(target_id, inode);
    if (inode_res.is_err())
      return inode_res.unwrap_error();

    auto inode_p = reinterpret_cast<Inode *>(inode.data());
    if (inode_p->get_type() == InodeType::Directory)
      return ErrorType::NotEmpty;
    // std::cout << "unlink: get type finish" << std::endl;
    auto remove_res = remove_file(target_id);
    // std::cout << "remove return" << std::endl;
    if (remove_res.is_err())
      return remove_res.unwrap_error();
    // std::cout << "remove finish" << std::endl;
    std::list<DirectoryEntry> list;
    read_directory(this, parent, list);
    std::string src = dir_list_to_string(list);
    src = rm_from_directory(src, name);
    this->write_file(parent, std::vector<u8>(src.begin(), src.end()));
    // TODO:
    // 1. Remove the file, you can use the function `remove_file`
    // 2. Remove the entry from the directory.
    // UNIMPLEMENTED();
    // std::cout << "unlink success" << std::endl;
    return KNullOk;
  }

} // namespace chfs
