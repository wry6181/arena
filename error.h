typedef enum {
  OPEN_FILE_ERROR = 0,
  READ_FILE_ERROR,
} log_error_value;

#define REGISTER_ERROR_VALUE(T) typedef T log_error;
REGISTER_ERROR_VALUE(log_error_value)
