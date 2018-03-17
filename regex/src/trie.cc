
#include "trie.h"
#include "trie_tree.h"

#define get_data() ((TrieTree *)mData)

Trie::Trie(uint32_t unit, bool lowercase)
   : mData((uintptr_t)(new TrieTree(unit, lowercase)))
{

}

Trie::~Trie()
{
	delete get_data();
}

bool Trie::add(const Slice& key, TUID id, bool whole_word)
{
	return get_data()->add(key, id, whole_word);
}

bool Trie::compile()
{
	return get_data()->compile();
}

bool Trie::check() const
{
	return get_data()->check();
}

Trie::TUID Trie::find_key(const Slice& key) const
{
	return get_data()->find_key(key);
}

void Trie::clear()
{
	get_data()->clear();
}

Trie::TUID Trie::find_first(const Slice& text) const
{
	return get_data()->find_first(text, DecodeType::kNone);
}

bool Trie::empty() const
{
	return get_data()->empty();
}

size_t Trie::words() const
{
	return get_data()->words();
}

size_t Trie::memory_size() const
{
	return get_data()->memory_size();
}

size_t Trie::wordset(Trie::value_set& store)
{
	BOOST_AUTO(trie, get_data());

	store.reserve(trie->words());

	for (BOOST_AUTO(iter, trie->begin()); iter != trie->end(); ++iter)
	{
		store.push_back(*iter);
	}

	return store.size();
}
