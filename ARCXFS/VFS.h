#pragma once
#include <string>
#include <string_view>
#include <Windows.h>
#include <unordered_map>
#include <arcx.h>

namespace VFS
{
	static std::hash<std::wstring_view> hasher;
	struct wchar_t_hasher
	{
		std::size_t operator()(wchar_t const*& s) const noexcept
		{
			return hasher(std::wstring_view(s));
		}
	};

	struct eq_to_wchar_t
	{
		bool operator()(const wchar_t const* &lhs, const wchar_t const* &rhs) const
		{
			return CompareStringEx(LOCALE_NAME_INVARIANT, 0, lhs, -1, rhs, -1, nullptr, nullptr, 0) == CSTR_EQUAL;
		}
	};

	class FSObject
	{
	public:
		virtual ~FSObject() = default;
		virtual bool is_file() = 0;
		virtual bool is_folder() = 0;
		virtual std::wstring& get_name() = 0;
	};

	class FSFolder : public FSObject
	{
	public:
		using folder_contents = std::unordered_map<std::wstring, FSObject*>;

		FSFolder(std::wstring name)
		{
			this->name = name;
		}

		bool is_file() override
		{
			return false;
		}

		bool is_folder() override
		{
			return true;
		}

		folder_contents& get_contents()
		{
			return contents;
		}

		std::wstring& get_name() override
		{
			return name;
		}

	private:
		std::wstring name;
		folder_contents contents;
	};

	class FSFile : public FSObject
	{
	public:
		FSFile(std::wstring name, ArcXFile *file)
		{
			this->name = name;
			this->file = file;
		}

		bool is_file() override
		{
			return true;
		}

		bool is_folder() override
		{
			return false;
		}

		ArcXFile *get_file() const
		{
			return file;
		}

		std::wstring& get_name() override
		{
			return name;
		}

	private:
		ArcXFile *file;
		std::wstring name;
	};

	static std::vector<std::wstring_view> split_path(wchar_t const* path)
	{
		std::vector<std::wstring_view> result;

		wchar_t c;
		auto start = path;
		uint64_t count = 0;
		while((c = *(path++)) != L'\0')
		{
			if(c == L'\\')
			{
				if (count != 0)
				{
					result.push_back(std::wstring_view(start, count));
					count = 0;
					start = path;
				}
			}
			else
				count++;
		}
		if(count != 0)
			result.push_back(std::wstring_view(start, count));
		return result;
	}

	static FSObject* traverse(FSFolder* root, std::wstring &path)
	{
		auto path_parts = split_path(path.c_str());

		FSObject *cur = root;

		for(auto part = path_parts.begin(); part != path_parts.end(); ++part)
		{
			if(cur->is_folder())
			{
				auto folder = reinterpret_cast<FSFolder*>(cur);
				auto contents = folder->get_contents();
				const auto val = contents.find(part->data());
				
				if (val == contents.end())
					return nullptr;
				cur = val->second;
			} 
			else if(part + 1 != path_parts.end())
				return nullptr;
		}

		return cur;
	}

	static FSFile *add_file(FSFolder *root, ArcXFile* file_ptr, wchar_t const* path)
	{
		auto path_parts = split_path(path);

		auto cur = root;

		for(auto part = path_parts.begin(); part + 1 != path_parts.end(); ++part)
		{
			auto contents = cur->get_contents();
			const auto item = contents.find(part->data());
			
			if (item == contents.end())
			{
				const auto new_folder = new FSFolder(part->data());
				contents[part->data()] = new_folder;
				cur = new_folder;
			}
			else if (item->second->is_file())
				return nullptr;
			else
				cur = reinterpret_cast<FSFolder *>(item->second);
		}

		auto file_name = path_parts.back();
		auto contents = cur->get_contents();
		const auto item = contents.find(file_name.data());

		if(item != contents.end())
			delete item->second;

		auto file = new FSFile(file_name.data(), file_ptr);
		cur->get_contents()[file_name.data()] = file;
		return file;
	}
}