#include <rosrect-listener-agent/backend_api.h>

using namespace utility;              // Common utilities like string conversions
using namespace web;                  // Common features like URIs.
using namespace web::http;            // Common HTTP functionality
using namespace web::http::client;    // HTTP client features
using namespace concurrency::streams; // Asynchronous streams
using namespace ::pplx;               // PPLX for tasks
using namespace web::json;            // JSON features

BackendApi::BackendApi()
{

  // std::cout << "Creating API instance..." << std::endl;
  // Check and set environment variables
  this->check_environment();

  // File variables
  this->log_dir = std::getenv("HOME");
  std::string run_id;
  bool check_id = ros::param::has("run_id");
  this->log_dir.append("/.cognicept/agent/logs/");
  std::string latest_log = this->log_dir + "latest_log_loc.txt";
  std::string disp_dir = "/$HOME/.cognicept/agent/logs/";

  if (check_id)
  {
    ros::param::get("run_id", run_id);
    this->log_dir.append(run_id);
    disp_dir.append(run_id);
    std::cout << "ROS session detected" << std::endl;
  }
  else
  {
    this->log_dir.append("unittest_logs");
    std::cout << "NO ROS session detected" << std::endl;
  }

  boost::filesystem::path dir2(this->log_dir);
  if (boost::filesystem::exists(dir2))
  {
    std::cout << "Agent log directory already exists: " << disp_dir << std::endl;
  }
  else
  {
    if (boost::filesystem::create_directories(dir2))
    {
      std::cout << "Agent log directory created: " << disp_dir << std::endl;
    }
  }

  // Write to file
  std::ofstream outfile;
  outfile.open(latest_log);
  outfile << std::setw(4) << disp_dir << std::endl;
  outfile.close();
  std::cout << "Updated latest log location in: "
            << "/$HOME/.cognicept/agent/logs/latest_log_loc.txt" << std::endl;

  this->log_name = this->log_dir + "/logData";
  this->log_ext = ".json";
  this->log_id = 0;

  if (this->agent_mode != "PROD")
  {
    std::cout << "TEST mode is ON. JSON Logs will be saved here: " << disp_dir << std::endl;
  }

  /* Error classification features in development below */
  // Error classification API variables

  if ((this->agent_type == "ERT") || (this->agent_type == "DB"))
  {
    // This configures the endpoint to ERT queries. Must be used only for testing. Undocumented.
    this->ecs_api_endpoint = "/api/ert/getErrorData/";
  }
  else if (this->agent_type == "ECS")
  {
    // This configures the endpoint to ECS queries. Production version.
    this->ecs_api_endpoint = "/api/ecs/getErrorData/";
  }
  else
  {
    // This means the agent is running in ROS mode. So no need to configure endpoint.
    this->ecs_api_endpoint = "/api/ert/getErrorData/";
  }
}

BackendApi::~BackendApi()
{

  // std::cout << "Logged out of API..." << std::endl;
}

void BackendApi::check_environment()
{
  // Other environment variables
  std::cout << "=======================Environment variables setup======================" << std::endl;
  // AGENT_TYPE
  if (std::getenv("AGENT_TYPE"))
  {
    // Success case
    this->agent_type = std::getenv("AGENT_TYPE");
    std::cout << "Environment variable AGENT_TYPE set to: " << this->agent_type << std::endl;
    // ECS_API, ECS_ROBOT_MODEL
    if ((this->agent_type == "DB") || (this->agent_type == "ERT") || (this->agent_type == "ECS"))
    {
      if (std::getenv("ECS_API"))
      {
        // Success case
        this->ecs_api_host = std::getenv("ECS_API");
        std::cout << "Environment variable ECS_API set to: " << this->ecs_api_host << std::endl;
        if (std::getenv("ECS_ROBOT_MODEL"))
        {
          // Success case
          this->ecs_robot_model = std::getenv("ECS_ROBOT_MODEL");
          std::cout << "Environment variable ECS_ROBOT_MODEL set to: " << this->ecs_robot_model << std::endl;
        }
        else
        {
          // Failure case - Default
          std::cerr << "Agent configured in " << this->agent_type << " mode but ECS_ROBOT_MODEL environment variable is not configured. Defaulting back to ROS mode instead..." << std::endl;
          this->agent_type = "ROS";
          this->agent_type = "ROS";
        }
      }
      else
      {
        // Failure case - Default
        std::cerr << "Agent configured in " << this->agent_type << " mode but ECS_API environment variable is not configured. Defaulting back to ROS mode instead..." << std::endl;
        this->agent_type = "ROS";
      }
    }
  }
  else
  {
    // Failure case - Default
    this->agent_type = "ROS";
    std::cerr << "Environment variable AGENT_TYPE unspecified. Defaulting to ROS mode..." << std::endl;
  }

  // ROBOT_CODE
  if (std::getenv("ROBOT_CODE"))
  {
    // Success case
    this->robot_id = std::getenv("ROBOT_CODE");
    std::cout << "Environment variable ROBOT_CODE set to: " << this->robot_id << std::endl;
  }
  else
  {
    // Failure case - Default
    this->robot_id = "Undefined";
    std::cerr << "Environment variable ROBOT_CODE unspecified. Defaulting to 'Undefined'..." << std::endl;
  }

  // SITE_CODE
  if (std::getenv("SITE_CODE"))
  {
    // Success case
    this->site_id = std::getenv("SITE_CODE");
    std::cout << "Environment variable SITE_CODE set to: " << this->site_id << std::endl;
  }
  else
  {
    // Failure case - Default
    this->site_id = "Undefined";
    std::cerr << "Environment variable SITE_CODE unspecified. Defaulting to 'Undefined'..." << std::endl;
  }

  // AGENT_ID
  if (std::getenv("AGENT_ID"))
  {
    // Success case
    this->agent_id = std::getenv("AGENT_ID");
    std::cout << "Environment variable AGENT_ID set to: " << this->agent_id << std::endl;
  }
  else
  {
    // Failure case - Default
    this->agent_id = "Undefined";
    std::cerr << "Environment variable AGENT_ID unspecified. Defaulting to 'Undefined'..." << std::endl;
  }

  // AGENT_MODE, AGENT_POST_API
  if (std::getenv("AGENT_MODE"))
  {
    // Success case
    this->agent_mode = std::getenv("AGENT_MODE");
    std::cout << "Environment variable AGENT_MODE set to: " << this->agent_mode << std::endl;
    // Specially handle POST_TEST case
    if (this->agent_mode == "POST_TEST")
    {
      if (std::getenv("AGENT_POST_API"))
      {
        // Success case
        this->agent_post_api = std::getenv("AGENT_POST_API");
        std::cout << "Environment variable AGENT_POST_API set to: " << this->agent_post_api << std::endl;
      }
      else
      {
        // Failure case - Default
        this->agent_mode = "JSON_TEST";
        std::cerr << "Agent configured in POST_TEST mode but AGENT_POST_API environment variable is not configured. Defaulting back to JSON_TEST mode instead..." << std::endl;
      }
    }
  }
  else
  {
    // Failure case - Default
    this->agent_mode = "JSON_TEST";
    std::cerr << "Environment variable AGENT_MODE unspecified. Defaulting to 'JSON_TEST'..." << std::endl;
  }

  std::cout << "=========================================================================" << std::endl;
}

pplx::task<void> BackendApi::post_event_log(json::value payload)
{
  std::cout << "Posting" << std::endl;

  return pplx::create_task([this, payload] {
           // Create HTTP client configuration
           http_client_config config;
           config.set_validate_certificates(false);

           // Create HTTP client
           http_client client(this->agent_post_api, config);

           // // Write the current JSON value to a stream with the native platform character width
           // utility::stringstream_t stream;
           // payload.serialize(stream);

           // // Display the string stream
           // std::cout << "Post data: " << stream.str() << std::endl;

           // Build request
           http_request req(methods::POST);
           // req.headers().add("Authorization", this->headers);
           req.set_request_uri("/api/agentstream/putRecord");
           req.set_body(payload);

           // Request ticket creation
           std::cout << "Pushing downstream..." << std::endl;
           return client.request(req);
         })
      .then([this](http_response response) {
        // If successful, print ticket details
        if (response.status_code() == status_codes::OK)
        {
          auto body = response.extract_string();
          std::wcout << "Response: " << body.get().c_str() << std::endl;
        }
        // If not, request failed
        else
        {
          std::cout << "Request failed" << std::endl;
        }
      });
}

void BackendApi::push_status(bool status, json::value telemetry)
{
  // Set all required info
  boost::posix_time::ptime utcTime = boost::posix_time::microsec_clock::universal_time();
  std::string timestr = to_iso_extended_string(utcTime);
  std::string level = "Heartbeat";
  std::string cflag = "Null";
  std::string module = "Status";
  std::string source = "Null";
  std::string message = "Null";
  std::string description = "Null";
  std::string resolution = "Null";
  std::string event_id = "Null";
  bool ticketBool = false;

  if (status)
  {
    message = "Online";
    ticketBool = false;
  }
  else
  {
    message = "Offline";
    ticketBool = true;
  }

  // Create JSON object
  json::value payload = json::value::object();

  // Create keys
  utility::string_t agentKey(U("agent_id"));
  utility::string_t roboKey(U("robot_id"));
  utility::string_t propKey(U("property_id"));
  utility::string_t eventidKey(U("event_id"));
  utility::string_t timeKey(U("timestamp"));
  utility::string_t msgKey(U("message"));
  utility::string_t lvlKey(U("level"));
  utility::string_t modKey(U("module"));
  utility::string_t srcKey(U("source"));
  utility::string_t cKey(U("compounding"));
  utility::string_t ticketKey(U("create_ticket"));
  utility::string_t descKey(U("description"));
  utility::string_t resKey(U("resolution"));
  utility::string_t telKey(U("telemetry"));

  // Assign key-value
  payload[agentKey] = json::value::string(U(this->agent_id));
  payload[roboKey] = json::value::string(U(this->robot_id));
  payload[propKey] = json::value::string(U(this->site_id));
  payload[eventidKey] = json::value::string(U(event_id));
  payload[timeKey] = json::value::string(U(timestr));
  payload[msgKey] = json::value::string(U(message));
  payload[lvlKey] = json::value::string(U(level));
  payload[modKey] = json::value::string(U(module));
  payload[srcKey] = json::value::string(U(source));
  payload[cKey] = json::value::string(U(cflag));
  payload[ticketKey] = json::value::boolean(U(ticketBool));
  payload[descKey] = json::value::string(U(description));
  payload[resKey] = json::value::string(U(resolution));
  payload[telKey] = telemetry;

  if (this->agent_mode == "JSON_TEST")
  {
    // Write the current JSON value to a stream with the native platform character width
    utility::stringstream_t stream;
    payload.serialize(stream);

    // Display the string stream
    // std::cout << stream.str() << std::endl;
    std::cout << "Status Logged: " << message << std::endl;

    // Write to file
    std::ofstream outfile;
    std::string filename = this->log_name + "Status" + this->log_ext;
    // std::cout << filename << std::endl;
    outfile.open(filename);
    outfile << std::setw(4) << stream.str() << std::endl;
    outfile.close();
  }
  else if (this->agent_mode == "POST_TEST")
  {
    // Write the current JSON value to a stream with the native platform character width
    utility::stringstream_t stream;
    payload.serialize(stream);

    // Display the string stream
    // std::cout << stream.str() << std::endl;
    std::cout << "Status Logged: " << message << std::endl;

    // Write to file
    std::ofstream outfile;
    std::string filename = this->log_name + "Status" + this->log_ext;
    // std::cout << filename << std::endl;
    outfile.open(filename);
    outfile << std::setw(4) << stream.str() << std::endl;
    outfile.close();

    // Post downstream
    this->post_event_log(payload).wait();
  }
}

void BackendApi::push_event_log(std::vector<std::vector<std::string>> log)
{
  // Create JSON payload and push to kinesis
  auto last_log = log.back();
  int idx = 0;

  // Get values for JSON
  std::string timestr = last_log[idx++];
  std::string level = last_log[idx++];
  std::string cflag = last_log[idx++];
  std::string module = last_log[idx++];
  std::string source = last_log[idx++];
  std::string message = last_log[idx++];
  std::string description = last_log[idx++];
  std::string resolution = last_log[idx++];
  std::string telemetry_str = last_log[idx++];
  std::string event_id = last_log[idx++];

  bool ticketBool = false;
  if (((level == "8") || (level == "16")) && ((cflag == "false") || (cflag == "Null")))
  {
    ticketBool = true;
  }
  else
  {
    ticketBool = false;
  }

  // Create JSON object
  json::value payload = json::value::object();

  // Create keys
  utility::string_t agentKey(U("agent_id"));
  utility::string_t roboKey(U("robot_id"));
  utility::string_t propKey(U("property_id"));
  utility::string_t eventidKey(U("event_id"));
  utility::string_t timeKey(U("timestamp"));
  utility::string_t msgKey(U("message"));
  utility::string_t lvlKey(U("level"));
  utility::string_t modKey(U("module"));
  utility::string_t srcKey(U("source"));
  utility::string_t cKey(U("compounding"));
  utility::string_t ticketKey(U("create_ticket"));
  utility::string_t descKey(U("description"));
  utility::string_t resKey(U("resolution"));
  utility::string_t telKey(U("telemetry"));

  // Assign key-value
  payload[agentKey] = json::value::string(U(this->agent_id));
  payload[roboKey] = json::value::string(U(this->robot_id));
  payload[propKey] = json::value::string(U(this->site_id));
  payload[eventidKey] = json::value::string(U(event_id));
  payload[timeKey] = json::value::string(U(timestr));
  payload[msgKey] = json::value::string(U(message));
  payload[lvlKey] = json::value::string(U(level));
  payload[modKey] = json::value::string(U(module));
  payload[srcKey] = json::value::string(U(source));
  if (cflag == "false")
  {
    payload[cKey] = json::value::boolean(U(false));
  }
  else if (cflag == "true")
  {
    payload[cKey] = json::value::boolean(U(true));
  }
  else
  {
    payload[cKey] = json::value::string(U("Null"));
  }
  payload[ticketKey] = json::value::boolean(U(ticketBool));
  payload[descKey] = json::value::string(U(description));
  payload[resKey] = json::value::string(U(resolution));
  payload[telKey] = json::value::parse(U(telemetry_str));

  if (this->agent_mode == "JSON_TEST")
  {
    // Write the current JSON value to a stream with the native platform character width
    utility::stringstream_t stream;
    payload.serialize(stream);

    // Display the string stream
    // std::cout << stream.str() << std::endl;
    std::cout << level << " level event logged with id: " << event_id << std::endl;

    // Write to file
    std::ofstream outfile;
    this->log_id++;
    std::string filename = this->log_name + std::to_string(this->log_id) + this->log_ext;
    std::cout << filename << std::endl;
    outfile.open(filename);
    outfile << std::setw(4) << stream.str() << std::endl;
    outfile.close();
  }
  else if (this->agent_mode == "POST_TEST")
  {
    // Write the current JSON value to a stream with the native platform character width
    utility::stringstream_t stream;
    payload.serialize(stream);

    // Display the string stream
    // std::cout << stream.str() << std::endl;
    std::cout << level << " level event logged with id: " << event_id << std::endl;

    // Write to file
    std::ofstream outfile;
    this->log_id++;
    std::string filename = this->log_name + std::to_string(this->log_id) + this->log_ext;
    std::cout << filename << std::endl;
    outfile.open(filename);
    outfile << std::setw(4) << stream.str() << std::endl;
    outfile.close();

    // Post downstream
    this->post_event_log(payload).wait();
  }
}

json::value BackendApi::create_event_log(std::vector<std::vector<std::string>> log)
{

  // Create JSON object
  json::value event_log = json::value::array();

  // Create keys
  utility::string_t cKey(U("Compounding"));
  utility::string_t timeKey(U("Date/Time"));
  utility::string_t descKey(U("Description"));
  utility::string_t lvlKey(U("Level"));
  utility::string_t msgKey(U("Message"));
  utility::string_t modKey(U("Module"));
  utility::string_t qidKey(U("QID"));
  utility::string_t resKey(U("Resolution"));
  utility::string_t eidKey(U("RobotEvent_ID"));
  utility::string_t srcKey(U("Source"));

  // std::cout << "Creating JSON log" << std::endl;
  for (int queue_id = 0; queue_id < log.size(); queue_id++)
  {

    // Get row
    auto current_row = log[queue_id];
    int idx = 0;

    // Retrieve data
    std::string timestr = current_row[idx++];
    std::string level = current_row[idx++];
    std::string cflag = current_row[idx++];
    std::string module = current_row[idx++];
    std::string source = current_row[idx++];
    std::string message = current_row[idx++];
    std::string description = current_row[idx++];
    std::string resolution = current_row[idx++];
    std::string event_id = current_row[idx++];
    std::string qidstr = std::to_string(queue_id);

    // Assign key-value
    event_log[queue_id][timeKey] = json::value::string(U(timestr));
    event_log[queue_id][lvlKey] = json::value::string(U(level));
    event_log[queue_id][cKey] = json::value::string(U(cflag));
    event_log[queue_id][modKey] = json::value::string(U(module));
    event_log[queue_id][srcKey] = json::value::string(U(source));
    event_log[queue_id][msgKey] = json::value::string(U(message));
    event_log[queue_id][descKey] = json::value::string(U(description));
    event_log[queue_id][resKey] = json::value::string(U(resolution));
    event_log[queue_id][eidKey] = json::value::string(U(event_id));
    event_log[queue_id][qidKey] = json::value::string(U(qidstr));
  }

  return (event_log);
}

/* Error Classification Features in development below */

pplx::task<void> BackendApi::query_error_classification(std::string msg_text)
{

  return pplx::create_task([this, msg_text] {
           // Create HTTP client configuration
           http_client_config config;
           config.set_validate_certificates(false);

           // Create HTTP client
           http_client client(this->ecs_api_host, config);

           // Build request
           http_request req(methods::GET);

           // Build request URI.
           uri_builder builder(this->ecs_api_endpoint);
           builder.append_query("RobotModel", this->ecs_robot_model);
           builder.append_query("ErrorText", msg_text);
           req.set_request_uri(builder.to_string());

           return client.request(req);
         })
      .then([this](http_response response) {
        // If successful, return JSON query
        if (response.status_code() == status_codes::OK)
        {
          auto body = response.extract_string();
          std::string body_str = body.get().c_str();
          this->msg_resp = body_str;
        }
        // If not, request failed
        else
        {
          std::cout << "Request failed" << std::endl;
        }
      });
}

json::value BackendApi::check_error_classification(std::string msg_text)
{

  this->query_error_classification(msg_text).wait();
  std::string temp_msg = this->msg_resp;
  json::value response = json::value::parse(temp_msg);
  json::value response_data;

  try
  {
    // std::cout << "Trying to get data..." << std::endl;
    response_data = response.at(U("data"));

    // // Write the current JSON value to a stream with the native platform character width
    // utility::stringstream_t stream;
    // response_data.serialize(stream);

    // Display the string stream
    // std::cout << "Get data: " << stream.str() << std::endl;
    return response_data[0];
  }
  catch (const json::json_exception &e)
  {
    // const std::exception& e
    // std::cout << " Can't get data, returning null" << std::endl;
    response_data = json::value::null();
    return response_data;
  }
}
