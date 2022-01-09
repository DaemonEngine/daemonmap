#include "confloader.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include <algorithm>

typedef std::vector<char> data_t;
class status_t
{
	typedef std::vector<char>::const_iterator cit;

	size_t error_count = 0;
	size_t column = 1;
	size_t line = 1;

	cit cursor;
	cit start;
	cit end;

	bool colon_found = false;

public:
	status_t( data_t const& data )
	: cursor( data.begin() ), start( data.begin() ), end( data.end() )
	{
	}

	bool no_key( void ) const
	{
		return !colon_found;
	}

	void key_found( void )
	{
		colon_found = true;
	}

	void error( char const* reason )
	{
		++error_count;
		assert( cursor - start <= INT_MAX );
		fprintf( stderr
				, "Error %zu at col=%zu, line=%zu: %s (\"%.*s\")\n"
				, error_count, line, column
				, reason
				, static_cast<int>( cursor - start ), &*start
				);
	}

	bool next( void )
	{
		if ( *cursor == '\n' )
		{
			++line;
			column = 0;
			colon_found = false;
		}
		++cursor;
		++column;
		return cursor >= end;
	}

	std::string get( void )
	{
		std::string ret;
		ret.assign( start, cursor );
		start = cursor;
		++start;
		return ret;
	}

	char ch( void ) const
	{
		return *cursor;
	}

	char st( void ) const
	{
		return *start;
	}

	bool success( void ) const
	{
		return error_count == 0;
	}
};

std::vector<char> slurp( char const* filename )
{
	std::vector<char> ret;
	FILE* src = fopen( filename, "r" );
	if ( !src )
	{
		fprintf( stderr, "failed to open file \"%s\": %s\n",
				filename, strerror( errno ) );
		return ret;
	}

	if ( -1 == fseek( src, 0, SEEK_END ) )
	{
		fprintf( stderr, "failed to seek EOF: %s\n",
				strerror( errno ) );
		fclose( src );
		return ret;
	}

	long file_sz = 0;
	if ( -1 == ( file_sz = ftell( src ) ) )
	{
		fprintf( stderr, "failed to get position: %s\n",
				strerror( errno ) );
		fclose( src );
		return ret;
	}

	rewind( src );
	assert( file_sz >= 0 );
	size_t file_size = static_cast<size_t>( file_sz );
	ret.resize( file_size );
	if ( 1 != fread( ret.data(), file_size, 1, src ) )
	{
		fprintf( stderr, "failed to read data: %s\n",
				strerror( errno ) );
	}
	fclose( src );
	return ret;
}

bool parse(
		std::vector<char> const& data,
		std::deque<record_t> &records,
		std::vector<std::string> &keys )
{
	size_t key = SIZE_MAX;
	size_t record = 0;
	status_t status( data );
	do
	{
		switch( status.ch() )
		{
			case ':':
				if ( status.no_key() && status.st() != ' ' )
				{
					status.key_found();
					std::string current_key = status.get();
					if ( current_key.end() != std::find_if( current_key.begin(), current_key.end(), []( char ch ){ return isspace( ch ); } ) )
					{
						status.error( "keys MUST NOT contain spaces" );
					}
					auto key_it = std::find( keys.begin(), keys.end(), current_key );
					if ( key_it != keys.end() )
					{
						key = static_cast<size_t>( key_it - keys.begin() );
					}
					else
					{
						keys.push_back( std::move( current_key ) );
						key = keys.size() - 1;
					}
				}
				break;
			case '\n':
				std::string val = status.get();
				if ( status.no_key() )
				{
					if ( val.empty() )
					{
						++record;
					}
					else
					{
						assert( !records.empty() );
						records.back().value += '\n';
						if ( val != " ." )
						{
							records.back().value += val;
						}
					}
				}
				else
				{
					record_t rc( { key, record, std::move( val ) } );
					records.push_back( std::move( rc ) );
				}
		}
	} while( !status.next() );

	return status.success();
}
