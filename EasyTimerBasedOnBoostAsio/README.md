EasyTimer which based on boost asio lib.

2017/11/12:
The coding is finished for now. Fix issue is ongoing.

2017/11/18:
Running ok now. Next step try to use boost::asio::io_service::work to replace keepalive timer.
The io_service::work can prevent run() call of boost::asio::io_service from returning when 
there is no more work to do.
