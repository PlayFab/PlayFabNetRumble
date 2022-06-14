#pragma once

namespace NetRunbleTools
{
	template <typename T>
	struct AsyncData
	{
		T callback;
	};

	template <typename Args>
	struct AsyncInvokerHelper
	{
		static_assert(std::is_pointer_v<Args>&& std::is_function_v<Args>, "Template parameter must be pointer to function.");
	};

	template <typename... Args>
	struct AsyncInvokerHelper<HRESULT(Args...)>
	{
		using ptr_type = HRESULT(*)(Args...);
		using arg_tuple = std::tuple<Args...>;
		static constexpr auto arity = sizeof...(Args);

		template <typename Is> struct Invoker;
		template <std::size_t... Is>
		struct Invoker<std::index_sequence<Is...>>
		{
			ptr_type f;
		};

		using type = Invoker<std::make_index_sequence<sizeof...(Args) - 1>>;
	};
	template<class... Args> struct AsyncInvokerHelper<HRESULT(__stdcall*)(Args...)> : public AsyncInvokerHelper<HRESULT(Args...)> {};
	template<class... Args> struct AsyncInvokerHelper<HRESULT(__stdcall*)(Args...) noexcept> : public AsyncInvokerHelper<HRESULT(Args...)> {};

	template <typename T, typename invoker = typename AsyncInvokerHelper<typename std::decay<T>::type>::type>
	inline invoker AsyncHelper(T&& fn) noexcept
	{
		return invoker{ fn };
	}


}
