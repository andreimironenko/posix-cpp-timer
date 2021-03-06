// C++ STL headers
#include <csignal>
#include <chrono>
#include <ratio>
#include <memory>
#include <functional>
#include <map>
#include <system_error>

#pragma once
namespace posixcpp {

  /**
   * C++17 wrapper for POSIX Interval Timer API.
   *
   * It is not:
   * - thread safe;
   * - copyable;
   * - movable;
   */
  class timer {

    class timer_;                             /**< Forward class reference to PIMPL implementation */
    std::shared_ptr<timer_> _timer; /**< pointer to PIMPL timer_ object */

    public:
    using callback_t = std::function<void(void*)>; /**< User provided callback function type*/

    /**
     * Defines all error codes for the timer class implementation
     */
    enum class error : int
    {
      // critical errors, decrease negative number to add a new error
      posix_timer_creation = -6,              /**< POSIX timer_create function call has failed */
      memcpy_failed = -5,                     /**< C-stdlib memcpy function call has failed */
      posix_timer_gettime = -4,               /**< POSIX timer_gettime function call has failed */
      posix_timer_settime = -3,               /**< POSIX timer_settime function call has failed */
      signal_handler_registration = -2,       /**< System signal handler registration has failed */
      unknown_error = -1,                     /**< Non identified system error */

      // warnings, increase number to add a new warning
      signal_handler_timer_null_pointer = 1,  /**< Signal handler receives null pointer to _timer object */
      signal_handler_unexpected_signal = 2,   /**< Signal handler receives unexpected signal ID */
      start_already_started = 3,              /**< User's attempt to start timer which is already running */
      resume_already_running = 4,             /**< User's attempt to resume timer which is already running */
      stop_while_not_running = 5,             /**< User's attempt to stop timer which is not running */
      suspend_while_not_running = 6           /**< User's attempt to suspend timer which is not running */
    };

    /**
     * The struct timer::error_category child  of std::error_category
     * It overrides and implements name and message functions.
     */
    struct error_category : std::error_category
    {
      /**
       * Overridden method return domain name for the given error_category struct
       *
       * @return  always return const char pointer to the name string
       */
      const char* name() const noexcept override
      {
        return "posixcpp-timer";
      }

      /**
       * Overridden method return error message string for the given error from timer::error enum
       */
      std::string message(int err) const override
      {
        static std::map<int, std::string> err2str =
        {
          {static_cast<int>(error::posix_timer_creation), "POSIX timer_create has failed"},
          {static_cast<int>(error::memcpy_failed), "std::memcpy has failed"},
          {static_cast<int>(error::posix_timer_gettime), "POSIX timer_gettime has failed"},
          {static_cast<int>(error::posix_timer_settime), "POSIX timer_settime has failed"},
          {static_cast<int>(error::signal_handler_registration), "SYSTEM sigaction has failed"},
          {static_cast<int>(error::unknown_error), "unknown error"},
          {static_cast<int>(error::signal_handler_timer_null_pointer), "signal_handler timer pointer is null"},
          {static_cast<int>(error::signal_handler_unexpected_signal), "signal_handler unexpected signal"},
          {static_cast<int>(error::start_already_started), "an attempt to start already running timer"},
          {static_cast<int>(error::resume_already_running), "an attempt to resume already running timer"},
          {static_cast<int>(error::stop_while_not_running), "an attempt to stop already stopped timer "},
          {static_cast<int>(error::suspend_while_not_running), "an attempt to stop already stopped timer "}
        };

        if (!err2str.count(err))
        {
          return std::string("Unknown error");
        }

        return err2str[err];
      }


      /**
       * The factory singleton class timer::error_category method.
       * If instance object does not exist, it's created. C++17 singleton pattern is used.
       * @return Always returns the same timer::error_category object reference.
       */
      static error_category& instance()
      {
        static error_category instance;
        return instance;
      }
    }; // struct error_category


    /**
     * @brief The explicit timer constructor.
     * The only mandatory parameter is period_sec, other parameters are optional and have default values.
     * The timeout period is defined by two parameters *period_sec* and *period_nsec*
     *  total_period = period_sec * 10^9 + period_nsec  nanoseconds
     * By default period_nsec = 0 and the user callback function is nullptr, but there is little sense to start a timer
     * without feedback to user.
     * I suggest provide at least these three parameters *period_sec*, *period_nsec* and your
     * *callback* function.
     *
     * To distinguish between callback function invocations, you might want to provide pointer to customisable data.
     * The pointer is passed as parameter to the customer *callback* function.
     * If *is_single_shot* parameters set to true, then the timer will run only once, by default it runs multiply
     * times until the timer
     *
     * @sa timer::suspend,  timer::stop.
     *
     * The last parameter *sig* specified signal can be used by POSIX library when the timer expires. By default it's
     * **SIGRTMAX**.
     *
     * @param period_sec      First part of timeout period in seconds, use literal 's' for std::chrono::seconds.
     * @param period_nsec     Second part of timeout period in nanoseconds, use literal 'ns' for
     *                        std::chrono::nanoseconds.
     * @param callback        User specified callback function, which is called when timer expires.
     * @param data            User specified pointer to the user data. This pointer is passed as argument to the
     *                        callback function.
     * @param is_single_shot  If this argument is true, then timer runs only once 
     * @param sig             It sets up the signal used by POSIX library, this signal is risen in the context of user
     *                        process.
     */
    explicit timer(std::chrono::seconds period_sec,
        std::chrono::nanoseconds period_nsec = static_cast<std::chrono::seconds>(0),
        callback_t callback = nullptr, void* data = nullptr,
        bool is_single_shot = false, int sig = SIGRTMAX
        );

    ~timer();

    timer(const timer&) = delete;
    timer(timer&&) = delete;
    timer& operator=(const timer&) = delete;
    timer& operator=(timer&&) = delete;

    void start();
    void reset();

    /**
     * suspend function
     */
    void suspend();

    void resume();

    /**
     * stop function
     */
    void stop();

    std::error_code try_start() noexcept;
    std::error_code try_reset() noexcept;
    std::error_code try_suspend() noexcept;
    std::error_code try_resume() noexcept;
    std::error_code try_stop() noexcept;
  }; // class timer

  inline std::error_code make_error_code(timer::error err) noexcept
  {
    return std::error_code(static_cast<int>(err), timer::error_category::instance());
  }

  inline bool is_warning(std::error_code ec)
  {
    return (ec && ec.value() > 0);
  }

}// namespace posixcpp

namespace std
{
  template <>
    struct is_error_code_enum<::posixcpp::timer::error> : true_type {};
} // namespace std
