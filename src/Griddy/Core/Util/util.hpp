
template <typename T>
inline void hash_combine (std::size_t& seed, const T& val)
{
 seed ^= std::hash<T>()(val) + 0x9e3779b9
 + (seed<<6) + (seed>>2);
} 