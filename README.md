# SQL C++ Wrapper

Header only C++ SQL Wrapper for Postgres database.

Usage examples can be found in `main.cpp`.

## Dependencies

```shell
sudo apt install libcppdb-postgresql0 libpq-dev
```

## Test

```shell
mkdir build
cd build
cmake ..
make -j
./test_app
```

## Usage

```cmake
cmake_minimum_required(VERSION 3.14)
project(my_app)

include(FetchContent)

FetchContent_Declare(
  sql_cpp_wrapper
  GIT_REPOSITORY https://github.com/socialdroids/sql_cpp_wrapper.git
  GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(sql_cpp_wrapper)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE sql_cpp_wrapper::sql_cpp_wrapper)
sql_wrapper_setup_drivers(my_app)
``` 

## References:
- [CppDB](https://cppcms.com/sql/cppdb/index.html) 
- [Postgres Backend](https://salsa.debian.org/debian/cppdb/-/blob/master/drivers/postgres_backend.cpp) 

### Database used for development:

```mermaid
erDiagram

	wifi_profiles {
		TEXT ssid
		TEXT password
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	hotspot_settings {
		SMALLINT id
		TEXT ssid
		TEXT password
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	robot_time_settings {
		SMALLINT id
		TIMESTAMPTZ configured_datetime
		BOOLEAN configured_datetime_use_utc
		BOOLEAN automatic_time_sync_enabled
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	robot_working_allowed_intervals {
		BIGSERIAL id
		TEXT weekday
		TIME start_time
		TIME end_time
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	maps {
		BIGSERIAL id
		TEXT pgm_path
		TEXT yaml_path
		TEXT stl_path
		TEXT obstacles_pgm_path
		DOUBLE  resolution
		INTEGER width
		INTEGER height
		BOOLEAN negate
		DOUBLE  occupied_thresh
		DOUBLE  free_thresh
		DOUBLE  origin_x
		DOUBLE  origin_y
		DOUBLE  origin_yaw
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	map_waypoints {
		BIGSERIAL id
		BIGINT map_id
		TEXT identifier
		DOUBLE x
		DOUBLE y
		DOUBLE yaw
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	map_polygons {
		BIGSERIAL id
		BIGINT map_id
		TEXT polygon_type
		TEXT identifier
		TIMESTAMPTZ created_at
		TIMESTAMPTZ updated_at
	}

	map_polygon_points {
    BIGINT polygon_id INTEGER point_index DOUBLE x DOUBLE y
	}
```
