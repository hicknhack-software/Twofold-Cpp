/* Twofold-Cpp
 * (C) Copyright 2025 HicknHack Software GmbH
 *
 * The original code can be found at:
 *     https://github.com/hicknhack-software/Twofold-Cpp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include "TargetBuilder.h"

#include <coroutine>
#include <string>
#include <utility>

namespace TwofoldRuntime {

template<class Promise = void>
struct UniqueCoroutineHandle final {
    using Handle = std::coroutine_handle<Promise>;

    UniqueCoroutineHandle() = default;
    explicit UniqueCoroutineHandle(Handle h) noexcept
        : m(h) {}
    UniqueCoroutineHandle(UniqueCoroutineHandle const&) = delete;
    UniqueCoroutineHandle& operator=(UniqueCoroutineHandle const&) = delete;
    UniqueCoroutineHandle(UniqueCoroutineHandle&& o) noexcept
        : m(std::exchange(o.m, nullptr)) {}
    UniqueCoroutineHandle& operator=(UniqueCoroutineHandle&& o) noexcept {
        destroy(m);
        m = std::exchange(o.m, nullptr);
        return *this;
    }
    ~UniqueCoroutineHandle() noexcept { destroy(m); }

    explicit operator bool() const noexcept { return static_cast<bool>(m); }
    operator Handle() noexcept { return m; }

    // auto extract() -> Handle { return std::exchange(m, nullptr); }

    void resume() const { m.resume(); }

private:
    static void destroy(Handle h) noexcept {
        if (h) h.destroy();
    }

private:
    Handle m = nullptr;
};

struct Append {
    std::string_view text;
    FilePosition originPosition;
};
struct AppendExpression {
    std::string_view text;
    FilePosition originPosition;
};
struct NewLine {
    FilePosition originPosition;
};
struct PushIndentation {
    std::string_view indent;
    FilePosition originPosition;
};
struct PopIndentation {};
struct IndentPart {
    std::string_view indent;
    FilePosition originPosition;
};
struct PushPartIndent {
    FilePosition originPosition;
};
struct PopPartIndent {};

struct Twofold {
    struct promise_type {
        TargetBuilder* builder{};

        auto initial_suspend() noexcept { return std::suspend_always{}; }
        auto final_suspend() noexcept { return std::suspend_always{}; }

        void unhandled_exception() {}
        void return_void() const noexcept {}

        auto get_return_object() {
            auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
            return Twofold{.handle = UniqueCoroutine{handle}};
        }

        auto yield_value(Append append) {
            builder->append(append.text, append.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(AppendExpression append) {
            builder->appendExpression(append.text, append.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(NewLine newLine) {
            builder->newLine(newLine.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(PushIndentation pushIndentation) {
            builder->pushIndentation(pushIndentation.indent, pushIndentation.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(PopIndentation) {
            builder->popIndentation();
            return std::suspend_never{};
        }
        auto yield_value(IndentPart indentPart) {
            builder->indentPart(indentPart.indent, indentPart.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(PushPartIndent pushPartIndent) {
            builder->pushPartIndent(pushPartIndent.originPosition);
            return std::suspend_never{};
        }
        auto yield_value(PopPartIndent) {
            builder->popPartIndent();
            return std::suspend_never{};
        }
    };
    using Coroutine = std::coroutine_handle<promise_type>;
    using UniqueCoroutine = UniqueCoroutineHandle<promise_type>;

    auto operator co_await() noexcept {
        struct Awaiter {
            Coroutine m_coroutine = {};

            bool await_ready() const noexcept { return false; }

            bool await_suspend(Coroutine awaitingCoroutine) const noexcept {
                m_coroutine.promise().builder = awaitingCoroutine.promise().builder;
                m_coroutine.resume(); // run called generator
                return false; // resume the awaitingCoroutine
            }
            void await_resume() const noexcept {}
        };
        return Awaiter{handle};
    }

    void setBuilder(TargetBuilder* builder) { static_cast<Coroutine>(handle).promise().builder = builder; }

    UniqueCoroutine handle{};
};

} // namespace TwofoldRuntime

using TwofoldRuntime::Twofold;
using TwofoldBuilder = TwofoldRuntime::TargetBuilder;
