#ifndef __DOCUMENT_LOADER_H__
#define __DOCUMENT_LOADER_H__

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/easy.h>
#include <curl/curlbuild.h>
#include <sstream>
#include <iostream>

#include "RemoteDocument.h"
#include "JsonLdError.h"

namespace json_spirit {
template<typename Object>
void read ( String, Object& );
}

template<typename Object>
class DocumentLoader {
	RemoteDocument<Object> loadDocument ( String url )  {
		RemoteDocument<Object> doc/* = new RemoteDocument*/ ( url, Object() );
		try {
			doc.setDocument ( fromURL ( /*new URL*/ ( url ) ) );
		} catch ( ... ) {
			throw JsonLdError ( LOADING_REMOTE_CONTEXT_FAILED, url );
		}
		return doc;
	}

	const Object& fromURL ( String url ) {
		Object r;
		json_spirit::read ( download ( url ), r );
		return r;
	}

	void* curl;
	static size_t write_data ( void *ptr, size_t size, size_t n, void *stream ) {
		String data ( ( const char* ) ptr, ( size_t ) size * n );
		* ( ( std::stringstream* ) stream ) << data << std::endl;
		return size * n;
	}

	String download ( const String& url ) {
		curl_easy_setopt ( curl, CURLOPT_URL, url.c_str() );
		curl_easy_setopt ( curl, CURLOPT_FOLLOWLOCATION, 1L );
		curl_easy_setopt ( curl, CURLOPT_NOSIGNAL, 1 );
		curl_easy_setopt ( curl, CURLOPT_ACCEPT_ENCODING, "deflate" );
		std::stringstream out;
		curl_easy_setopt ( curl, CURLOPT_WRITEFUNCTION, write_data );
		curl_easy_setopt ( curl, CURLOPT_WRITEDATA, &out );
		CURLcode res = curl_easy_perform ( curl );
		if ( res != CURLE_OK ) throw std::runtime_error ( "curl_easy_perform() failed: "s + curl_easy_strerror ( res ) );
		return out.str();
	}

public:
	DocumentLoader() {
		curl = curl_easy_init();
	}
	virtual ~DocumentLoader() {
		curl_easy_cleanup ( curl );
	}
};
#endif
