#pragma once

/** Singleton template class ×*/

template<class T>
class Singleton
{
public:
	static T* Instance()
	{
		if (NULL == _member)
		{
			_member = new T();
		}
		return _member;
	}

	static void Release()
	{
		if (_member)
		{
			delete _member;
			_member = NULL;
		}
	}

private:
	Singleton(){}

	~Singleton()
	{
		Release();
	}

	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

private:
	static T* _member;
};

template<class T>
T* Singleton<T>::_member = NULL;