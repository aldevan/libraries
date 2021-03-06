// Copyright (c) 2013, Kirk Shoop (kirk.shoop@gmail.com)
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, 
//  are permitted provided that the following conditions are met:
//
//  - Redistributions of source code must retain the above copyright notice, 
//      this list of conditions and the following disclaimer.
//  - Redistributions in binary form must reproduce the above copyright notice, 
//      this list of conditions and the following disclaimer in the documentation 
//      and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#ifndef UNIQUE_ERROR_SOURCE
#define UNIQUE_ERROR_SOURCE

#ifndef UNIQUE_ERROR_NAMESPACE
#error UNIQUE_ERROR_NAMESPACE must be defined
#endif

namespace UNIQUE_ERROR_NAMESPACE
{
	namespace cmn=COMMON_NAMESPACE;

	class exception : public std::exception
	{
	public:

		explicit 
		exception(const std::string& message)
			: std::exception(message.c_str())
		{
		}

		explicit 
		exception(const char* message)
			: std::exception(message)
		{
		}

	};

	template<typename ErrorTag>
	class specific_exception : public exception
	{
	public:
		typedef
			decltype(unique_error_default(ErrorTag()))
		type;

		specific_exception(type valueArg, const std::string& message)
			: exception(message.c_str())
			, value(valueArg)
		{
		}

		explicit 
		specific_exception(type valueArg, const char* message = NULL)
			: exception(message)
			, value(valueArg)
		{
		}

		type get() const 
		{
			return value;
		}

	private:
		type value;
	};

	struct Disposition
	{
		enum type {
			Invalid = 0,

			Initiated,
			Defaulted,
			Suppressed,
			Checked,
			Unchecked,

			End,
			Begin = Initiated
		};
	};

	template<typename ErrorTag>
	class unique_error
	{
	public:
		typedef 
			unique_error
		this_type;

		typedef
			ErrorTag
		tag;

		typedef
			decltype(unique_error_default(tag()))
		type;

		typedef
			specific_exception<tag>
		exception;

		class static_error
		{
		private:
			friend class unique_error<tag>;

			template<typename ErrorTag>
			friend bool operator<(const unique_error<ErrorTag>& lhs, const typename unique_error<ErrorTag>::static_error& rhs);

			template<typename ErrorTag>
			friend bool operator==(const unique_error<ErrorTag>& lhs, const typename unique_error<ErrorTag>::static_error& rhs);

			template<typename ErrorTag>
			friend bool operator<(const typename unique_error<ErrorTag>::static_error& lhs, const unique_error<ErrorTag>& rhs);

			type value;
		};

		static static_error cast(type raw)
		{
			static_error result;
			result.value = raw;
			return result;
		}

		static this_type make(type raw)
		{
			this_type result;
			result.reset(raw);
			return result;
		}

		static this_type make(const static_error& other)
		{
			return this_type(other);
		}

		~unique_error()
		{
			reset();
		}

		unique_error()
			: value(unique_error_default(tag()))
			, disposition(Disposition::Defaulted)
		{
		}

		unique_error(static_error other)
			: value(other.value)
			, disposition(Disposition::Initiated)
		{
			if (value != unique_error_default(tag()))
			{
				unique_error_report_initiated(value, tag());
			}
		}

		unique_error(unique_error&& other)
			: value(std::move(other.suppress().release()))
			, disposition(Disposition::Unchecked)
		{
		}

		unique_error(const unique_error& other)
			: value(other.value)
			, disposition(Disposition::Unchecked)
		{
		}

		void swap(unique_error& other)
		{
			using std::swap;
			swap(value, other.value);
			swap(disposition, other.disposition);
		}

		unique_error& operator=(unique_error other)
		{
			swap(other);
			return *this;
		}

		operator typename cmn::unspecified_bool<this_type>::type() const
		{
			return cmn::unspecified_bool<this_type>::get(ok());
		}

		this_type& reset()
		{
			reset(unique_error_default(tag()));
			disposition = Disposition::Defaulted;
			return *this;
		}

		this_type& reset(type raw)
		{
			if (disposition == Disposition::Unchecked)
			{
				std::terminate();
			}

			value = raw;
			disposition = Disposition::Unchecked;

			if (value != unique_error_default(tag()))
			{
				unique_error_report_reset(value, tag());
			}

			return *this;
		}

		type release()
		{
			type result = value;
			reset();
			return result;
		}

		type get() const
		{
			return value;
		}

		bool try_ok() const
		{
			return unique_error_ok(value, tag());
		}

		bool ok() const
		{
			disposition = Disposition::Checked;
			return try_ok();
		}

		const this_type& suppress() const
		{
			disposition = Disposition::Suppressed;
			return *this;
		}

		this_type& suppress() 
		{
			disposition = Disposition::Suppressed;
			return *this;
		}

		void throw_if(const std::string& message) const
		{
			if (!ok())
			{
				throw exception(value, message);
			}
		}

		void throw_if(const char* message = NULL) const
		{
			if (!ok())
			{
				throw exception(value, message);
			}
		}

	private:
		template<typename ErrorTag>
		friend bool operator<(const unique_error<ErrorTag>& lhs, const unique_error<ErrorTag>& rhs);

		template<typename ErrorTag>
		friend bool operator==(const unique_error<ErrorTag>& lhs, const unique_error<ErrorTag>& rhs);

		template<typename ErrorTag>
		friend bool operator<(const unique_error<ErrorTag>& lhs, const typename unique_error<ErrorTag>::static_error& rhs);

		template<typename ErrorTag>
		friend bool operator==(const unique_error<ErrorTag>& lhs, const typename unique_error<ErrorTag>::static_error& rhs);

		template<typename ErrorTag>
		friend bool operator<(const typename unique_error<ErrorTag>::static_error& lhs, const unique_error<ErrorTag>& rhs);

	private:
		type value;
		mutable Disposition::type disposition;
	};

	template<typename ErrorTag>
	void swap(unique_error<ErrorTag>& lhs, unique_error<ErrorTag>& rhs)
	{
		lhs.swap(rhs);
	}

	template<typename ErrorTag>
	bool operator<(const unique_error<ErrorTag>& lhs, const unique_error<ErrorTag>& rhs)
	{
		return lhs.value < rhs.value;
	}

	template<typename ErrorTag>
	bool operator==(const unique_error<ErrorTag>& lhs, const unique_error<ErrorTag>& rhs)
	{
		return lhs.value == rhs.value;
	}

	template<typename ErrorTag>
	bool operator!=(const unique_error<ErrorTag>& lhs, const unique_error<ErrorTag>& rhs)
	{
		return !(lhs == rhs);
	}

	template<typename ErrorTag>
	bool operator<(const unique_error<ErrorTag>& lhs, const typename unique_error<ErrorTag>::static_error& rhs)
	{
		return lhs.value < rhs.value;
	}

	template<typename ErrorTag>
	bool operator==(const unique_error<ErrorTag>& lhs, const typename unique_error<ErrorTag>::static_error& rhs)
	{
		return lhs.value == rhs.value;
	}

	template<typename ErrorTag>
	bool operator!=(const unique_error<ErrorTag>& lhs, const typename unique_error<ErrorTag>::static_error& rhs)
	{
		return !(lhs == rhs);
	}

	template<typename ErrorTag>
	bool operator<(const typename unique_error<ErrorTag>::static_error& lhs, const unique_error<ErrorTag>& rhs)
	{
		return lhs.value < rhs.value;
	}

	template<typename ErrorTag>
	bool operator==(const typename unique_error<ErrorTag>::static_error& lhs, const unique_error<ErrorTag>& rhs)
	{
		return rhs == lhs;
	}

	template<typename ErrorTag>
	bool operator!=(const typename unique_error<ErrorTag>::static_error& lhs, const unique_error<ErrorTag>& rhs)
	{
		return !(rhs == lhs);
	}
}

#endif // UNIQUE_ERROR_SOURCE