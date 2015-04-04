/**
    A container object to maintain state relating to JsonLdOptions and the
    current Context, and push these into the relevant algorithms in
    JsonLdProcessor as necessary.

    @author tristan
*/
class JsonLdApi {

	JsonLdOptions opts;
	Object value = null;
	Context context = null;

	/**
	    Constructs an empty JsonLdApi object using the default JsonLdOptions, and
	    without initialization.
	*/
public: JsonLdApi() {
		this ( new JsonLdOptions ( "" ) );
	}

	/**
	    Constructs a JsonLdApi object using the given object as the initial
	    JSON-LD object, and the given JsonLdOptions.

	    @param input
	              The initial JSON-LD object.
	    @param opts
	              The JsonLdOptions to use.
	    @
	               If there is an error initializing using the object and
	               options.
	*/
	JsonLdApi ( Object input, JsonLdOptions opts )  {
		this ( opts );
		initialize ( input, null );
	}

	/**
	    Constructs a JsonLdApi object using the given object as the initial
	    JSON-LD object, the given context, and the given JsonLdOptions.

	    @param input
	              The initial JSON-LD object.
	    @param context
	              The initial context.
	    @param opts
	              The JsonLdOptions to use.
	    @
	               If there is an error initializing using the object and
	               options.
	*/
	JsonLdApi ( Object input, Object context, JsonLdOptions opts )  {
		this ( opts );
		initialize ( input, null );
	}

	/**
	    Constructs an empty JsonLdApi object using the given JsonLdOptions, and
	    without initialization. <br>
	    If the JsonLdOptions parameter is null, then the default options are
	    used.

	    @param opts
	              The JsonLdOptions to use.
	*/
	JsonLdApi ( JsonLdOptions opts ) {
		if ( opts == null )
			opts = new JsonLdOptions ( "" ); else
			this.opts = opts;
	}

	/**
	    Initializes this object by cloning the input object using
	    {@link JsonLdUtils#clone(Object)}, and by parsing the context using
	    {@link Context#parse(Object)}.

	    @param input
	              The initial object, which is to be cloned and used in
	              operations.
	    @param context
	              The context object, which is to be parsed and used in
	              operations.
	    @
	               If there was an error cloning the object, or in parsing the
	               context.
	*/
private: void initialize ( Object input, Object context_ )  {
		if ( input.isList() || input.isMap() )
			value = clone ( input );
		// TODO: string/IO input
		context = Context ( opts );
		if ( context_ != null )
			context = context.parse ( context_ );
	}

	/***
	    ____ _ _ _ _ _ _ / ___|___ _ __ ___ _ __ __ _ ___| |_ / \ | | __ _ ___ _
	    __(_) |_| |__ _ __ ___ | | / _ \| '_ ` _ \| '_ \ / _` |/ __| __| / _ \ |
	    |/ _` |/ _ \| '__| | __| '_ \| '_ ` _ \ | |__| (_) | | | | | | |_) | (_|
	    | (__| |_ / ___ \| | (_| | (_) | | | | |_| | | | | | | | | \____\___/|_|
	    |_| |_| .__/ \__,_|\___|\__| /_/ \_\_|\__, |\___/|_| |_|\__|_| |_|_| |_|
	    |_| |_| |___/
	*/

	/**
	    Compaction Algorithm

	    http://json-ld.org/spec/latest/json-ld-api/#compaction-algorithm

	    @param activeCtx
	              The Active Context
	    @param activeProperty
	              The Active Property
	    @param element
	              The current element
	    @param compactArrays
	              True to compact arrays.
	    @return The compacted JSON-LD object.
	    @
	               If there was an error during compaction.
	*/
	Object compact ( Context& activeCtx, String& activeProperty, Object& element_, boolean compactArrays )  {
		if ( element_.isList() ) { // 2
			List<Object> result, e = element_.list(); // 2.1
			for ( Object item : e ) { // 2.2
				Object compactedItem = compact ( activeCtx, activeProperty, item, compactArrays ); // 2.2.1
				if ( compactedItem != null ) result.push_back ( compactedItem ); // 2.2.2
			}
			return ( compactArrays && result.size() == 1 && activeCtx.getContainer ( activeProperty ) == null ) ? result.get ( 0 ) : return result; // 2.3, 2.4
		}

		if ( !element.isMap() ) return _element; // 1
		map_t elem = element_.obj(); // 3
		if ( elem.containsKey ( "@value" ) || elem.containsKey ( "@id" ) ) { // 4
			Object compactedValue = activeCtx.compactValue ( activeProperty, elem );
			if ( ! ( compactedValue.isMap() || compactedValue.isList() ) )
				return compactedValue;
		}
		const boolean insideReverse = ( "@reverse"s == activeProperty ); // 5
		Map<String, Object> result; //6
		for ( auto x : elem ) { // 7
			String expandedProperty = x.first;
			Object expandedValue = x.second; //elem.get ( expandedProperty );

			if ( is ( expandedProperty, { "@id"s, "@type"s} ) ) { // 7.1
				Object compactedValue;
				if ( expandedValue.isString() ) // 7.1.1
					compactedValue = activeCtx.compactIri ( expandedValue.str(), "@type"s == expandedProperty );
				else { // 7.1.2
					List<Object/*String*/> types;
					for ( Object/*String*/ expandedType : expandedValue.list() ) // 7.1.2.2)
						types.push_back ( activeCtx.compactIri ( expandedType, true ) );
					if ( types.size() == 1 ) compactedValue = types.get ( 0 ); // 7.1.2.3)
					else compactedValue = types;
				}
				const String alias = activeCtx.compactIri ( expandedProperty, true );// 7.1.3)
				result.put ( alias, compactedValue );// 7.1.4)
				continue;
				// TODO: old add value code, see if it's still relevant?
				// addValue(rval, alias, compactedValue,
				// isArray(compactedValue)
				// && ((List<Object>) expandedValue).size() == 0);
			}


			if ( "@reverse"s == expandedProperty ) {// 7.2)
				Map<String, Object> compactedValue = compact ( activeCtx, "@reverse", expandedValue, compactArrays ).obj();// 7.2.1)
				// Note: Must create a new set to avoid modifying the set we
				// are iterating over
				for ( auto x : compactedValue ) {// 7.2.2)
					String property = x.first;
					const Object value = x.second;//compactedValue.get ( property );
					if ( activeCtx.isReverseProperty ( property ) ) {	// 7.2.2.1)
						if ( !compactArrays ||  "@set"s == ( activeCtx.getContainer ( property ) )	// 7.2.2.1.1)
						        && !value.isList() ) {
							List<Object> tmp;
							tmp.push_back ( value );
							result.put ( property, tmp );
						}
						if ( !result.containsKey ( property ) )	// 7.2.2.1.2)
							result.put ( property, value );
						else {	// 7.2.2.1.3)
							if ( ! result.get ( property ).isList() ) {
								List<Object> tmp;
								tmp.push_back ( result.put ( property, tmp ) );
							}
							if ( value.isList() ) result.get ( property ).list().push_back ( value.list().begin(), value.list.end() );
							else result.get ( property ).list().push_back ( value );
						}
						compactedValue.remove ( property );// 7.2.2.1.4)
					}
				}
				if ( !compactedValue.isEmpty() ) // 7.2.3)
					result.put ( activeCtx.compactIri ( "@reverse", true ), compactedValue );	// 7.2.3.1-2)
				continue;	// 7.2.4)
			}

			if ( "@index"s == expandedProperty && "@index"s == activeCtx.getContainer ( activeProperty ) ) continue; // 7.3
			else if ( is ( espandedProperty, {"@index"s, "@value"s, "@language"s} ) {	// 7.4)
			result.put ( activeCtx.compactIri ( expandedProperty, true ), expandedValue );	// 7.4.1-2)
				continue;
			}

			// NOTE: expanded value must be an array due to expansion
			// algorithm.
			if ( !expandedValue.list().size() ) { // 7.5
			String itemActiveProperty = activeCtx.compactIri ( expandedProperty, expandedValue, true, insideReverse );	// 7.5.1)
				if ( !result.containsKey ( itemActiveProperty ) )	// 7.5.2)
					result.put ( itemActiveProperty, ArrayList<Object>() );
				else {
					Object value = result.get ( itemActiveProperty );
					if ( ! value.isList() ) {
						List<Object> tmp;
						tmp.push_back ( value );
						result.put ( itemActiveProperty, tmp );
					}
				}
			}

			for ( Object expandedItem : expandedValue.list() ) {	// 7.6)
			String itemActiveProperty = activeCtx.compactIri ( expandedProperty, expandedItem, true, insideReverse );	// 7.6.1)
				String container = activeCtx.getContainer ( itemActiveProperty );	// 7.6.2)
				// get @list value if appropriate
				const boolean isList = expandedItem.isMap() && expandedItem.obj( ) .containsKey ( "@list" );
				Object list;
				if ( isList ) list = expandedItem.obj( ).get ( "@list" );
				Object compactedItem = compact ( activeCtx, itemActiveProperty, isList ? list : expandedItem, compactArrays );	// 7.6.3)
				if ( isList ) {  // 7.6.4)
					if ( ! compactedItem.isList() ) { // 7.6.4.1)
						List<Object> tmp;
						tmp.push_back ( compactedItem );
						compactedItem = tmp;
					}
					// 7.6.4.2)
					if ( !"@list"s == container ) { // 7.6.4.2)
						Map<String, Object> wrapper; // 7.6.4.2.1)
						// TODO: SPEC: no mention of vocab = true
						wrapper.put ( activeCtx.compactIri ( "@list", true ), compactedItem );
						compactedItem = wrapper;
						if ( expandedItem.obj( ).containsKey ( "@index" ) ) { // 7.6.4.2.2)
							compactedItem.obj( ).put (
							    // TODO: SPEC: no mention of vocab =
							    // true
							    activeCtx.compactIri ( "@index", true ),
							    expandedItem.obj( ).get ( "@index" ) );
						}
					} else if ( result.containsKey ( itemActiveProperty ) ) // 7.6.4.3)
						throw JsonLdError ( Error.COMPACTION_TO_LIST_OF_LISTS,
						                    "There cannot be two list objects associated with an active property that has a container mapping" );
				}
				if ( is ( container, { "@language"s, "@index"s } ) { // 7.6.5)
				Map<String, Object> mapObject; // 7.6.5.1)
				if ( result.containsKey ( itemActiveProperty ) )
						mapObject = result.get ( itemActiveProperty ).obj();
					else
						result.put ( itemActiveProperty, mapObject );
					if ( "@language"s ==  container && compactedItem.isMap() && compactedItem.obj() .containsKey ( "@value" ) ) // 7.6.5.2)
						compactedItem = compactedItem.obj( ).get ( "@value" );
					String mapKey = expandedItem.obj() .get ( container ).str(); // 7.6.5.3)
					if ( !mapObject.containsKey ( mapKey ) ) // 7.6.5.4)
						mapObject.put ( mapKey, compactedItem );
					else {
						List<Object> tmp;
						if ( ! mapObject.get ( mapKey ).isList() ) tmp.push_back ( mapObject.put ( mapKey, tmp ) );
						else tmp = mapObject.get ( mapKey ).list();
						tmp.push_back ( compactedItem );
					}
				}
				else { // 7.6.6)
					Boolean check = !compactArrays; // 7.6.6.1)
					check |= is ( container, { "@set"s, "@list"s} ) || is ( expandedProperty, { "@list"s, "@graph"s} ) );
					check &= !compactedItem.isList();
					if ( check ) {
					List<Object> tmp;
					tmp.push_back ( compactedItem );
						compactedItem = tmp;
					}
					if ( !result.containsKey ( itemActiveProperty ) ) // 7.6.6.2)
						result.put ( itemActiveProperty, compactedItem );
						else {
							if ( ! result.get ( itemActiveProperty ).isList() ) {
								List<Object> tmp; // TODO: UNUSED
								tmp.push_back ( result.put ( itemActiveProperty, tmp ) );
							}
							if ( compactedItem.isList() )
								result.get ( itemActiveProperty ) .list().push_back ( compactedItem.list.begin(), compactedItem.list.end() );
							else
								result.get ( itemActiveProperty ).list( ).push_back ( compactedItem );
						}

				}
			}
		}
		// 8)
		return result;
	}

	/**
	    Compaction Algorithm

	    http://json-ld.org/spec/latest/json-ld-api/#compaction-algorithm

	    @param activeCtx
	              The Active Context
	    @param activeProperty
	              The Active Property
	    @param element
	              The current element
	    @return The compacted JSON-LD object.
	    @
	               If there was an error during compaction.
	*/
	Object compact ( Context activeCtx, String activeProperty, Object element ) {
		return compact ( activeCtx, activeProperty, element, true );
	}

	/**
	    Expansion Algorithm

	    http://json-ld.org/spec/latest/json-ld-api/#expansion-algorithm

	    @param activeCtx
	              The Active Context
	    @param activeProperty
	              The Active Property
	    @param element
	              The current element
	    @return The expanded JSON-LD object.
	    @
	               If there was an error during expansion.
	*/
	Object expand ( Context activeCtx, String activeProperty, Object element ) {
		if ( element == null ) return null; // 1
		if ( element.isList() ) { // 3
			List<Object> result; // 3.1
			for ( Object item : element.list() ) { // 3.2
				Object v = expand ( activeCtx, activeProperty, item ); // 3.2.1
				if ( ( "@list"s == ( activeProperty ) || "@list"s == ( activeCtx // 3.2.2
				        .getContainer ( activeProperty ) ) )
				        && ( v.isList() || ( v.isMap() && v.obj( )
				                             .containsKey ( "@list" ) ) ) )
					throw JsonLdError ( Error.LIST_OF_LISTS, "lists of lists are not permitted." );
				else if ( v != null ) { // 3.2.3
					if ( v.isList() )
						result.push_back ( v.list().begin(), v.list().end() );
					else
						result.push_back ( v );
				}
			}
			return result; // 3.3)
		} else if ( element.isMap() ) { // 4
			Map<String, Object> elem = element.map();
			if ( elem.containsKey ( "@context" ) ) activeCtx = activeCtx.parse ( elem.get ( "@context" ) ); // 5
			Map<String, Object> result; // 6
			//			List<String> keys = new ArrayList<String> ( elem.keySet() ); // 7
			//			Collections.sort ( keys );
			for ( /*const String key : keys*/ auto x : elem ) {
				String key = x.first;
				const Object value = x.second;//elem.get ( key );
				if ( key.equals ( "@context" ) ) continue; // 7.1
				const String expandedProperty = activeCtx.expandIri ( key, false, true, null, null ); // 7.2
				Object expandedValue;
				if ( expandedProperty == null // 7.3
				        || ( expandedProperty.find ( ':' ) == String::npos && !isKeyword ( expandedProperty ) ) ) continue;
				if ( isKeyword ( expandedProperty ) ) { // 7.4
					if ( "@reverse"s ==  activeProperty  ) // 7.4.1
						throw JsonLdError ( Error.INVALID_REVERSE_PROPERTY_MAP, "a keyword cannot be used as a @reverse propery" );
					if ( result.containsKey ( expandedProperty ) ) // 7.4.2
						throw JsonLdError ( Error.COLLIDING_KEYWORDS, expandedProperty + " already exists in result" );
					if ( "@id"s ==  expandedProperty ) { // 7.4.3
						if ( !  value.isString()  ) throw JsonLdError ( Error.INVALID_ID_VALUE, "value of @id must be a string" );
						expandedValue = activeCtx .expandIri ( ( String ) value, true, false, null, null );
					} else if ( "@type"s == ( expandedProperty ) ) { // 7.4.4)
						if ( value.isList() ) {
							expandedValue = ArrayList<String>();
							for ( const Object v :  value.list() ) {
								if ( ! v.isString() ) throw JsonLdError ( Error.INVALID_TYPE_VALUE, "@type value must be a string or array of strings" );
								expandedValue.list( ).push_back ( activeCtx.expandIri ( ( String ) v,
								                                  true, true, null, null ) );
							}
						} else if ( value.isString() )
							expandedValue = activeCtx.expandIri ( ( String ) value, true, true, null, null );
						// TODO: SPEC: no mention of empty map check
						else if ( value.isMap() ) {
							if (  value.obj( ).size() != 0 )
								throw JsonLdError ( Error.INVALID_TYPE_VALUE, "@type value must be a an empty object for framing" );
							expandedValue = value;
						} else
							throw JsonLdError ( Error.INVALID_TYPE_VALUE, "@type value must be a string or array of strings" );
					} else if ( "@graph"s == expandedProperty ) expandedValue = expand ( activeCtx, "@graph", value ); // 7.4.5
					else if ( "@value"s == ( expandedProperty ) ) { // 7.4.6
						if ( value != null && ( value.isMap() || value.isList() ) )
							throw JsonLdError ( Error.INVALID_VALUE_OBJECT_VALUE, "value of "
							                    + expandedProperty + " must be a scalar or null" );
						expandedValue = value;
						if ( expandedValue == null ) {
							result.put ( "@value", null );
							continue;
						}
					} else if ( "@language"s == expandedProperty ) { // 7.4.7
						if ( ! ( value.isString() ) )
							throw JsonLdError ( Error.INVALID_LANGUAGE_TAGGED_STRING, "Value of " + expandedProperty + " must be a string" );
						expandedValue = value.str( ).tolower();
					} else if ( "@index"s == expandedProperty ) { // 7.4.8
						if ( ! value.isString() ) throw JsonLdError ( Error.INVALID_INDEX_VALUE, "Value of " + expandedProperty + " must be a string" );
						expandedValue = value;
					} else if ( "@list"s == ( expandedProperty ) ) { // 7.4.9
						if ( activeProperty == null || "@graph"s == ( activeProperty ) ) continue; // 7.4.9.1
						expandedValue = expand ( activeCtx, activeProperty, value ); // 7.4.9.2

						// NOTE: step not in the spec yet
						if ( ! expandedValue.isList() ) {
							List<Object> tmp;
							tmp.push_back ( expandedValue );
							expandedValue = tmp;
						}

						for ( const Object o : expandedValue.list() ) // 7.4.9.3
							if ( o.isMap() && o.obj( ).containsKey ( "@list" ) )
								throw JsonLdError ( Error.LIST_OF_LISTS, "A list may not contain another list" );
					} else if ( "@set"s == expandedProperty ) expandedValue = expand ( activeCtx, activeProperty, value ); // 7.4.10
					else if ( "@reverse"s == expandedProperty ) { // 7.4.11
						if ( ! value.isMap() ) throw JsonLdError ( Error.INVALID_REVERSE_VALUE, "@reverse value must be an object" );
						expandedValue = expand ( activeCtx, "@reverse", value ); // 7.4.11.1)
						// NOTE: algorithm assumes the result is a map
						// 7.4.11.2)
						if ( expandedValue.obj( ).containsKey ( "@reverse" ) ) {
							Map<String, Object> reverse = expandedValue.obj( ) .get ( "@reverse" ).obj();
							for ( const String property : reverse.keySet() ) {
								const Object item = reverse.get ( property );
								if ( !result.containsKey ( property ) ) result.put ( property, ArrayList<Object>() ); // 7.4.11.2.1)
								if ( item.isList() ) result.get ( property ).list() .push_back ( item.list().begin(), item.list().end() ); // 7.4.11.2.2)
								else result.get ( property ).list().push_back ( item );
							}
							// 7.4.11.3)
							if (  expandedValue.obj().size() > ( expandedValue.obj().containsKey ( "@reverse" ) ? 1 : 0 ) ) {
								if ( !result.containsKey ( "@reverse" ) ) result.put ( "@reverse", map_t() ); // 7.4.11.3.1)
								Map<String, Object> reverseMap = result .get ( "@reverse" ).obj(); // 7.4.11.3.2)
								for ( auto x : expandedValue.map( ) ) { // 7.4.11.3.3)
									String property = x.first;
									if ( "@reverse"s == ( property ) ) continue;
									List<Object> items = expandedValue.obj( ) .get ( property ).list(); // 7.4.11.3.3.1)
									for ( const Object item : items ) {
										// 7.4.11.3.3.1.1)
										if ( item.isMap() && ( item.obj( ).containsKey ( "@value" ) || item.obj() .containsKey ( "@list" ) ) )
											throw JsonLdError ( Error.INVALID_REVERSE_PROPERTY_VALUE );
										if ( !reverseMap.containsKey ( property ) ) reverseMap.put ( property, ArrayList<Object>() ); // 7.4.11.3.3.1.2)
										reverseMap.get ( property ).list( ).push_back ( item ); // 7.4.11.3.3.1.3
									}
								}
							}
							continue; // 7.4.11.4)
						}
						// TODO: SPEC no mention of @explicit etc in spec
						else if ( is ( expandedProperty, { "@explicit"s, "@default"s, "@embed"s, "@embedChildren"s, "@omitDefault"s } ) )
							expandedValue = expand ( activeCtx, expandedProperty, value );
						if ( expandedValue != null ) result.put ( expandedProperty, expandedValue ); // 7.4.12)
						continue; // 7.4.13)
					} else if ( "@language"s == ( activeCtx.getContainer ( key ) ) && value.isMap() ) { // 7.5
						expandedValue = ArrayList<Object>(); // 7.5.1
						for ( auto x : value.obj( ) ) { // 7.5.2
							String language = x.first;
							Object languageValue = value.obj( ).get ( language );
							if ( ! languageValue.isList() ) { // 7.5.2.1)
								Object tmp = languageValue;
								languageValue = ArrayList<Object>();
								languageValue.list().push_back ( tmp );
							}
							for ( const Object item : languageValue.list() ) { // 7.5.2.2)
								if ( ! item.isString() ) // 7.5.2.2.1)
									throw JsonLdError ( Error.INVALID_LANGUAGE_MAP_VALUE, "Expected " + item.toString() + " to be a string" );
								// 7.5.2.2.2)
								Map<String, Object> tmp;
								tmp.put ( "@value", item );
								tmp.put ( "@language", language.tolower() );
								expandedValue.list().push_back ( tmp );
							}
						}
					}
					// 7.6)
					else if ( "@index"s == ( activeCtx.getContainer ( key ) ) && value.isMap() ) {
						expandedValue = ArrayList<Object>(); // 7.6.1
						//					List<String> indexKeys = ArrayList<String> ( value().obj().keySet() ); // 7.6.2
						//					Collections.sort ( indexKeys );
						for ( auto x : value.obj() ) { //const String index : indexKeys ) {
							String index = x.first;
							Object indexValue = x.second;//( ( Map<String, Object> ) value ).get ( index );
							if ( ! indexValue.isList() ) { // 7.6.2.1
								Object tmp = indexValue;
								indexValue = ArrayList<Object>();
								indexValue.list( ).push_back ( tmp );
							}
							indexValue = expand ( activeCtx, key, indexValue ); // 7.6.2.2)
							for ( Map<String, Object> item : ( List<Map<String, Object>> ) indexValue ) { // 7.6.2.3)
								if ( !item.containsKey ( "@index" ) ) item.put ( "@index", index ); // 7.6.2.3.1)
								expandedValue.list().push_back ( item ); // 7.6.2.3.2)
							}
						}
					} else expandedValue = expand ( activeCtx, key, value ); // 7.7
					if ( expandedValue == null ) continue; // 7.8
					if ( "@list"s == ( activeCtx.getContainer ( key ) ) ) { // 7.9
						if ( ! ( expandedValue.isMap() )
						        || ! ( ( Map<String, Object> ) expandedValue ).containsKey ( "@list" ) ) {
							Object tmp = expandedValue;
							if ( ! ( tmp.isList() ) ) {
								tmp = new ArrayList<Object>();
								( ( List<Object> ) tmp ).add ( expandedValue );
							}
							expandedValue = map_t();
							expandedValue.obj( ).put ( "@list", tmp );
						}
					}
					// 7.10)
					if ( activeCtx.isReverseProperty ( key ) ) {
						if ( !result.containsKey ( "@reverse" ) ) result.put ( "@reverse", map_t ); // 7.10.1)
						Map<String, Object> reverseMap =  result .get ( "@reverse" ).obj(); // 7.10.2)
						if ( ! expandedValue.isList() ) { // 7.10.3)
							Object tmp = expandedValue;
							expandedValue = ArrayList<Object>();
							expandedValue.list().push_back ( tmp );
						}
						// 7.10.4)
						for ( const Object item : ( List<Object> ) expandedValue ) {
							// 7.10.4.1)
							if ( item.isMap() && (  item.obj( ).containsKey ( "@value" ) ||  item.obj() .containsKey ( "@list" ) ) )
								throw JsonLdError ( Error.INVALID_REVERSE_PROPERTY_VALUE );
							// 7.10.4.2)
							if ( !reverseMap.containsKey ( expandedProperty ) ) reverseMap.put ( expandedProperty, ArrayList<Object>() );
							// 7.10.4.3)
							if ( item.isList() ) reverseMap.get ( expandedProperty ).list().push_back ( item.list().begin(), item.list.end() );
							else reverseMap.get ( expandedProperty ) .list().push_back ( item );
						}
					} else { // 7.11
						// 7.11.1)
						if ( !result.containsKey ( expandedProperty ) ) result.put ( expandedProperty, ArrayList<Object>() );
						// 7.11.2
						if ( expandedValue.isList() ) result.get ( expandedProperty ).list().push_back ( expandedValue.list().begin(), expandedValue.list().end() );
						else result.get ( expandedProperty ) list().push_back ( expandedValue );
					}
				}
				// 8)
				if ( result.containsKey ( "@value" ) ) {
					// 8.1)
					// TODO: is this method faster than just using containsKey for
					// each?
					const Set<String> keySet = new HashSet ( result.keySet() );
					keySet.remove ( "@value" );
					keySet.remove ( "@index" );
					const boolean langremoved = keySet.remove ( "@language" );
					const boolean typeremoved = keySet.remove ( "@type" );
					if ( ( langremoved && typeremoved ) || !keySet.isEmpty() ) {
						throw JsonLdError ( Error.INVALID_VALUE_OBJECT,
						                    "value object has unknown keys" );
					}
					// 8.2)
					const Object rval = result.get ( "@value" );
					if ( rval == null ) {
						// nothing else is possible with result if we set it to
						// null, so simply return it
						return null;
					}
					// 8.3)
					if ( ! ( rval.isString() ) && result.containsKey ( "@language" ) ) {
						throw JsonLdError ( Error.INVALID_LANGUAGE_TAGGED_VALUE,
						                    "when @language is used, @value must be a string" );
					}
					// 8.4)
					else if ( result.containsKey ( "@type" ) ) {
						// TODO: is this enough for "is an IRI"
						if ( ! ( result.get ( "@type" ).isString() )
						        || ( ( String ) result.get ( "@type" ) ).startsWith ( "_:" )
						        || ! ( ( String ) result.get ( "@type" ) ).contains ( ":" ) ) {
							throw JsonLdError ( Error.INVALID_TYPED_VALUE,
							                    "value of @type must be an IRI" );
						}
					}
				}
				// 9)
				else if ( result.containsKey ( "@type" ) ) {
					const Object rtype = result.get ( "@type" );
					if ( ! ( rtype.isList() ) ) {
						const List<Object> tmp = new ArrayList<Object>();
						tmp.add ( rtype );
						result.put ( "@type", tmp );
					}
				}
				// 10)
				else if ( result.containsKey ( "@set" ) || result.containsKey ( "@list" ) ) {
					// 10.1)
					if ( result.size() > ( result.containsKey ( "@index" ) ? 2 : 1 ) ) {
						throw JsonLdError ( Error.INVALID_SET_OR_LIST_OBJECT,
						                    "@set or @list may only contain @index" );
					}
					// 10.2)
					if ( result.containsKey ( "@set" ) ) {
						// result becomes an array here, thus the remaining checks
						// will never be true from here on
						// so simply return the value rather than have to make
						// result an object and cast it with every
						// other use in the function.
						return result.get ( "@set" );
					}
				}
				// 11)
				if ( result.containsKey ( "@language" ) && result.size() == 1 )
					result = null;
				// 12)
				if ( activeProperty == null || "@graph"s == ( activeProperty ) ) {
					// 12.1)
					if ( result != null
					        && ( result.size() == 0 || result.containsKey ( "@value" ) || result
					             .containsKey ( "@list" ) ) )
						result = null;
					// 12.2)
					else if ( result != null && result.containsKey ( "@id" ) && result.size() == 1 )
						result = null;
				}
				// 13)
				return result;
			}
			// 2) If element is a scalar
			else {
				// 2.1)
				if ( activeProperty == null || "@graph"s == ( activeProperty ) )
					return null;
				return activeCtx.expandValue ( activeProperty, element );
			}
		}

		/**
		    Expansion Algorithm

		    http://json-ld.org/spec/latest/json-ld-api/#expansion-algorithm

		    @param activeCtx
		              The Active Context
		    @param element
		              The current element
		    @return The expanded JSON-LD object.
		    @
		               If there was an error during expansion.
		*/
public: Object expand ( Context activeCtx, Object element )  {
			return expand ( activeCtx, null, element );
		}

		/***
		    _____ _ _ _ _ _ _ _ _ | ___| | __ _| |_| |_ ___ _ __ / \ | | __ _ ___ _
		    __(_) |_| |__ _ __ ___ | |_ | |/ _` | __| __/ _ \ '_ \ / _ \ | |/ _` |/ _
		    \| '__| | __| '_ \| '_ ` _ \ | _| | | (_| | |_| || __/ | | | / ___ \| |
		    (_| | (_) | | | | |_| | | | | | | | | |_| |_|\__,_|\__|\__\___|_| |_| /_/
		    \_\_|\__, |\___/|_| |_|\__|_| |_|_| |_| |_| |___/
		*/

		void generateNodeMap ( Object element, Map<String, Object> nodeMap )  {
			generateNodeMap ( element, nodeMap, "@default", null, null, null );
		}

		void generateNodeMap ( Object element, Map<String, Object> nodeMap, String activeGraph ) {
			generateNodeMap ( element, nodeMap, activeGraph, null, null, null );
		}

		void generateNodeMap ( Object element, Map<String, Object> nodeMap, String activeGraph,
		                       Object activeSubject, String activeProperty, Map<String, Object> list ) {
			// 1)
			if ( element.isList() ) {
				// 1.1)
				for ( const Object item : ( List<Object> ) element )
					generateNodeMap ( item, nodeMap, activeGraph, activeSubject, activeProperty, list );
				return;
			}

			// for convenience
			const Map<String, Object> elem = ( Map<String, Object> ) element;

			// 2)
			if ( !nodeMap.containsKey ( activeGraph ) )
				nodeMap.put ( activeGraph, newMap() );
			const Map<String, Object> graph = ( Map<String, Object> ) nodeMap.get ( activeGraph );
			Map<String, Object> node = ( Map<String, Object> ) ( activeSubject == null ? null : graph
			                           .get ( activeSubject ) );

			// 3)
			if ( elem.containsKey ( "@type" ) ) {
				// 3.1)
				List<String> oldTypes;
				const List<String> newTypes = new ArrayList<String>();
				if ( elem.get ( "@type" ).isList() )
					oldTypes = ( List<String> ) elem.get ( "@type" ); else {
					oldTypes = new ArrayList<String>();
					oldTypes.add ( ( String ) elem.get ( "@type" ) );
				}
				for ( const String item : oldTypes ) {
					if ( item.startsWith ( "_:" ) )
						newTypes.add ( generateBlankNodeIdentifier ( item ) ); else
						newTypes.add ( item );
				}
				if ( elem.get ( "@type" ).isList() )
					elem.put ( "@type", newTypes ); else
					elem.put ( "@type", newTypes.get ( 0 ) );
			}

			// 4)
			if ( elem.containsKey ( "@value" ) ) {
				// 4.1)
				if ( list == null )
					JsonLdUtils.mergeValue ( node, activeProperty, elem );
				// 4.2)
				else
					JsonLdUtils.mergeValue ( list, "@list", elem );
			}

			// 5)
			else if ( elem.containsKey ( "@list" ) ) {
				// 5.1)
				const Map<String, Object> result = newMap ( "@list", new ArrayList<Object>() );
				// 5.2)
				// for (const Object item : (List<Object>) elem.get("@list")) {
				// generateNodeMap(item, nodeMap, activeGraph, activeSubject,
				// activeProperty, result);
				// }
				generateNodeMap ( elem.get ( "@list" ), nodeMap, activeGraph, activeSubject, activeProperty,
				                  result );
				// 5.3)
				JsonLdUtils.mergeValue ( node, activeProperty, result );
			}

			// 6)
			else {
				// 6.1)
				String id = ( String ) elem.remove ( "@id" );
				if ( id != null ) {
					if ( id.startsWith ( "_:" ) )
						id = generateBlankNodeIdentifier ( id );
				}
				// 6.2)
				else
					id = generateBlankNodeIdentifier ( null );
				// 6.3)
				if ( !graph.containsKey ( id ) ) {
					const Map<String, Object> tmp = newMap ( "@id", id );
					graph.put ( id, tmp );
				}
				// 6.4) TODO: SPEC this line is asked for by the spec, but it breaks
				// various tests
				// node = (Map<String, Object>) graph.get(id);
				// 6.5)
				if ( activeSubject.isMap() ) {
					// 6.5.1)
					JsonLdUtils.mergeValue ( ( Map<String, Object> ) graph.get ( id ), activeProperty,
					                         activeSubject );
				}
				// 6.6)
				else if ( activeProperty != null ) {
					const Map<String, Object> reference = newMap ( "@id", id );
					// 6.6.2)
					if ( list == null ) {
						// 6.6.2.1+2)
						JsonLdUtils.mergeValue ( node, activeProperty, reference );
					}
					// 6.6.3) TODO: SPEC says to add ELEMENT to @list member, should
					// be REFERENCE
					else
						JsonLdUtils.mergeValue ( list, "@list", reference );
				}
				// TODO: SPEC this is removed in the spec now, but it's still needed
				// (see 6.4)
				node = ( Map<String, Object> ) graph.get ( id );
				// 6.7)
				if ( elem.containsKey ( "@type" ) ) {
					for ( const Object type : ( List<Object> ) elem.remove ( "@type" ) )
						JsonLdUtils.mergeValue ( node, "@type", type );
				}
				// 6.8)
				if ( elem.containsKey ( "@index" ) ) {
					const Object elemIndex = elem.remove ( "@index" );
					if ( node.containsKey ( "@index" ) ) {
						if ( !JsonLdUtils.deepCompare ( node.get ( "@index" ), elemIndex ) )
							throw JsonLdError ( Error.CONFLICTING_INDEXES );
					} else
						node.put ( "@index", elemIndex );
				}
				// 6.9)
				if ( elem.containsKey ( "@reverse" ) ) {
					// 6.9.1)
					const Map<String, Object> referencedNode = newMap ( "@id", id );
					// 6.9.2+6.9.4)
					const Map<String, Object> reverseMap = ( Map<String, Object> ) elem
					                                       .remove ( "@reverse" );
					// 6.9.3)
					for ( const String property : reverseMap.keySet() ) {
						const List<Object> values = ( List<Object> ) reverseMap.get ( property );
						// 6.9.3.1)
						for ( const Object value : values ) {
							// 6.9.3.1.1)
							generateNodeMap ( value, nodeMap, activeGraph, referencedNode, property, null );
						}
					}
				}
				// 6.10)
				if ( elem.containsKey ( "@graph" ) )
					generateNodeMap ( elem.remove ( "@graph" ), nodeMap, id, null, null, null );
				// 6.11)
				const List<String> keys = new ArrayList<String> ( elem.keySet() );
				Collections.sort ( keys );
				for ( String property : keys ) {
					const Object value = elem.get ( property );
					// 6.11.1)
					if ( property.startsWith ( "_:" ) )
						property = generateBlankNodeIdentifier ( property );
					// 6.11.2)
					if ( !node.containsKey ( property ) )
						node.put ( property, new ArrayList<Object>() );
					// 6.11.3)
					generateNodeMap ( value, nodeMap, activeGraph, id, property, null );
				}
			}
		}

		/**
		    Blank Node identifier map specified in:

		    http://www.w3.org/TR/json-ld-api/#generate-blank-node-identifier
		*/
private: const Map<String, String> blankNodeIdentifierMap = new LinkedHashMap<String, String>();

		/**
		    Counter specified in:

		    http://www.w3.org/TR/json-ld-api/#generate-blank-node-identifier
		*/
private: int blankNodeCounter = 0;

		/**
		    Generates a blank node identifier for the given key using the algorithm
		    specified in:

		    http://www.w3.org/TR/json-ld-api/#generate-blank-node-identifier

		    @param id
		              The id, or null to generate a fresh, unused, blank node
		              identifier.
		    @return A blank node identifier based on id if it was not null, or a
		           fresh, unused, blank node identifier if it was null.
		*/
		String generateBlankNodeIdentifier ( String id ) {
			if ( id != null && blankNodeIdentifierMap.containsKey ( id ) )
				return blankNodeIdentifierMap.get ( id );
			const String bnid = "_:b" + blankNodeCounter++;
			if ( id != null )
				blankNodeIdentifierMap.put ( id, bnid );
			return bnid;
		}

		/**
		    Generates a fresh, unused, blank node identifier using the algorithm
		    specified in:

		    http://www.w3.org/TR/json-ld-api/#generate-blank-node-identifier

		    @return A fresh, unused, blank node identifier.
		*/
		String generateBlankNodeIdentifier() {
			return generateBlankNodeIdentifier ( null );
		}

		/***
		    _____ _ _ _ _ _ _ | ___| __ __ _ _ __ ___ (_)_ __ __ _ / \ | | __ _ ___ _
		    __(_) |_| |__ _ __ ___ | |_ | '__/ _` | '_ ` _ \| | '_ \ / _` | / _ \ |
		    |/ _` |/ _ \| '__| | __| '_ \| '_ ` _ \ | _|| | | (_| | | | | | | | | | |
		    (_| | / ___ \| | (_| | (_) | | | | |_| | | | | | | | | |_| |_| \__,_|_|
		    |_| |_|_|_| |_|\__, | /_/ \_\_|\__, |\___/|_| |_|\__|_| |_|_| |_| |_|
		    |___/ |___/
		*/

private: class FramingContext {
		public: boolean embed;
		public: boolean explicit;
		public: boolean omitDefault;

		public: FramingContext() {
				embed = true;
				explicit = false;
				omitDefault = false;
				embeds = null;
			}

		public: FramingContext ( JsonLdOptions opts ) {
				this();
				if ( opts.getEmbed() != null )
					this.embed = opts.getEmbed();
				if ( opts.getExplicit() != null )
					this.explicit = opts.getExplicit();
				if ( opts.getOmitDefault() != null )
					this.omitDefault = opts.getOmitDefault();
			}

		public: Map<String, EmbedNode> embeds = null;
		}

private: class EmbedNode {
		public: Object parent = null;
		public: String property = null;
		}

private: Map<String, Object> nodeMap;

		/**
		    Performs JSON-LD <a
		    href="http://json-ld.org/spec/latest/json-ld-framing/">framing</a>.

		    @param input
		              the expanded JSON-LD to frame.
		    @param frame
		              the expanded JSON-LD frame to use.
		    @return the framed output.
		    @
		               If the framing was not successful.
		*/
public: List<Object> frame ( Object input, List<Object> frame )  {
			// create framing state
			const FramingContext state = new FramingContext ( this.opts );

			// use tree map so keys are sotred by default
			const Map<String, Object> nodes = new TreeMap<String, Object>();
			generateNodeMap ( input, nodes );
			this.nodeMap = ( Map<String, Object> ) nodes.get ( "@default" );

			const List<Object> framed = new ArrayList<Object>();
			// NOTE: frame validation is done by the function not allowing anything
			// other than list to me passed
			frame ( state,
			        this.nodeMap,
			        ( frame != null && frame.size() > 0 ? ( Map<String, Object> ) frame.get ( 0 ) : newMap() ),
			        framed, null );

			return framed;
		}

		/**
		    Frames subjects according to the given frame.

		    @param state
		              the current framing state.
		    @param subjects
		              the subjects to filter.
		    @param frame
		              the frame.
		    @param parent
		              the parent subject or top-level array.
		    @param property
		              the parent property, initialized to null.
		    @
		               If there was an error during framing.
		*/
private: void frame ( FramingContext state, Map<String, Object> nodes, Map<String, Object> frame,
		                      Object parent, String property )  {

			// filter out subjects that match the frame
			const Map<String, Object> matches = filterNodes ( state, nodes, frame );

			// get flags for current frame
			Boolean embedOn = getFrameFlag ( frame, "@embed", state.embed );
			const Boolean explicicOn = getFrameFlag ( frame, "@explicit", state.explicit );

			// add matches to output
			const List<String> ids = new ArrayList<String> ( matches.keySet() );
			Collections.sort ( ids );
			for ( const String id : ids ) {
				if ( property == null )
					state.embeds = new LinkedHashMap<String, EmbedNode>();

				// start output
				const Map<String, Object> output = newMap();
				output.put ( "@id", id );

				// prepare embed meta info
				const EmbedNode embeddedNode = new EmbedNode();
				embeddedNode.parent = parent;
				embeddedNode.property = property;

				// if embed is on and there is an existing embed
				if ( embedOn && state.embeds.containsKey ( id ) ) {
					const EmbedNode existing = state.embeds.get ( id );
					embedOn = false;

					if ( existing.parent.isList() ) {
						for ( const Object p : ( List<Object> ) existing.parent ) {
							if ( JsonLdUtils.compareValues ( output, p ) ) {
								embedOn = true;
								break;
							}
						}
					}
					// existing embed's parent is an object
					else {
						if ( ( ( Map<String, Object> ) existing.parent ).containsKey ( existing.property ) ) {
							for ( const Object v : ( List<Object> ) ( ( Map<String, Object> ) existing.parent )
							        .get ( existing.property ) ) {
								if ( v.isMap
								        && Obj.equals ( id, ( ( Map<String, Object> ) v ).get ( "@id" ) ) ) {
									embedOn = true;
									break;
								}
							}
						}
					}

					// existing embed has already been added, so allow an overwrite
					if ( embedOn )
						removeEmbed ( state, id );
				}

				// not embedding, add output without any other properties
				if ( !embedOn )
					addFrameOutput ( state, parent, property, output ); else {
					// add embed meta info
					state.embeds.put ( id, embeddedNode );

					// iterate over subject properties
					const Map<String, Object> element = ( Map<String, Object> ) matches.get ( id );
					List<String> props = new ArrayList<String> ( element.keySet() );
					Collections.sort ( props );
					for ( const String prop : props ) {

						// copy keywords to output
						if ( isKeyword ( prop ) ) {
							output.put ( prop, JsonLdUtils.clone ( element.get ( prop ) ) );
							continue;
						}

						// if property isn't in the frame
						if ( !frame.containsKey ( prop ) ) {
							// if explicit is off, embed values
							if ( !explicicOn )
								embedValues ( state, element, prop, output );
							continue;
						}

						// add objects
						const List<Object> value = ( List<Object> ) element.get ( prop );

						for ( const Object item : value ) {

							// recurse into list
							if ( ( item.isMap() )
							        && ( ( Map<String, Object> ) item ).containsKey ( "@list" ) ) {
								// add empty list
								const Map<String, Object> list = newMap();
								list.put ( "@list", new ArrayList<Object>() );
								addFrameOutput ( state, output, prop, list );

								// add list objects
								for ( const Object listitem : ( List<Object> ) ( ( Map<String, Object> ) item )
								        .get ( "@list" ) ) {
									// recurse into subject reference
									if ( JsonLdUtils.isNodeReference ( listitem ) ) {
										const Map<String, Object> tmp = newMap();
										const String itemid = ( String ) ( ( Map<String, Object> ) listitem )
										                      .get ( "@id" );
										// TODO: nodes may need to be node_map,
										// which is global
										tmp.put ( itemid, this.nodeMap.get ( itemid ) );
										frame ( state, tmp,
										        ( Map<String, Object> ) ( ( List<Object> ) frame.get ( prop ) )
										        .get ( 0 ), list, "@list" );
									} else {
										// include other values automatcially (TODO:
										// may need JsonLdUtils.clone(n))
										addFrameOutput ( state, list, "@list", listitem );
									}
								}
							}

							// recurse into subject reference
							else if ( JsonLdUtils.isNodeReference ( item ) ) {
								const Map<String, Object> tmp = newMap();
								const String itemid = ( String ) ( ( Map<String, Object> ) item ).get ( "@id" );
								// TODO: nodes may need to be node_map, which is
								// global
								tmp.put ( itemid, this.nodeMap.get ( itemid ) );
								frame ( state, tmp,
								        ( Map<String, Object> ) ( ( List<Object> ) frame.get ( prop ) ).get ( 0 ),
								        output, prop );
							} else {
								// include other values automatically (TODO: may
								// need JsonLdUtils.clone(o))
								addFrameOutput ( state, output, prop, item );
							}
						}
					}

					// handle defaults
					props = new ArrayList<String> ( frame.keySet() );
					Collections.sort ( props );
					for ( const String prop : props ) {
						// skip keywords
						if ( isKeyword ( prop ) )
							continue;

						const List<Object> pf = ( List<Object> ) frame.get ( prop );
						Map<String, Object> propertyFrame = pf.size() > 0 ? ( Map<String, Object> ) pf
						                                    .get ( 0 ) : null;
						if ( propertyFrame == null )
							propertyFrame = newMap();
						const boolean omitDefaultOn = getFrameFlag ( propertyFrame, "@omitDefault",
						                              state.omitDefault );
						if ( !omitDefaultOn && !output.containsKey ( prop ) ) {
							Object def = "@null";
							if ( propertyFrame.containsKey ( "@default" ) )
								def = JsonLdUtils.clone ( propertyFrame.get ( "@default" ) );
							if ( ! def.isList() ) {
								List<Object> tmp;
								tmp.add ( def );
								def = tmp;
							}
							const Map<String, Object> tmp1 = newMap ( "@preserve", def );
							const List<Object> tmp2 = new ArrayList<Object>();
							tmp2.add ( tmp1 );
							output.put ( prop, tmp2 );
						}
					}

					// add output to parent
					addFrameOutput ( state, parent, property, output );
				}
			}
		}

		Boolean getFrameFlag ( Map<String, Object> frame, String name, boolean thedefault ) {
			Object value = frame.get ( name );
			if ( value.isList() && value.list( ).size() ) value = value.list( ).get ( 0 );
			if ( value.isMap() && value.obj( ).containsKey ( "@value" ) ) value.obj( ).get ( "@value" );
			if ( value.isBoolean() ) return value.get_bool();
			return thedefault;
		}

		/**
		    Removes an existing embed.

		    @param state
		              the current framing state.
		    @param id
		              the @id of the embed to remove.
		*/
		static void removeEmbed ( FramingContext state, String id ) {
			// get existing embed
			const Map<String, EmbedNode> embeds = state.embeds;
			const EmbedNode embed = embeds.get ( id );
			const Object parent = embed.parent;
			const String property = embed.property;

			// create reference to replace embed
			const Map<String, Object> node = newMap ( "@id", id );

			// remove existing embed
			if ( JsonLdUtils.isNode ( parent ) ) {
				// replace subject with reference
				const List<Object> newvals = new ArrayList<Object>();
				const List<Object> oldvals = ( List<Object> ) ( ( Map<String, Object> ) parent )
				                             .get ( property );
				for ( const Object v : oldvals ) {
					if ( v.isMap() && Obj.equals ( ( ( Map<String, Object> ) v ).get ( "@id" ), id ) )
						newvals.add ( node ); else
						newvals.add ( v );
				}
				( ( Map<String, Object> ) parent ).put ( property, newvals );
			}
			// recursively remove dependent dangling embeds
			removeDependents ( embeds, id );
		}

		static void removeDependents ( Map<String, EmbedNode> embeds, String id ) {
			// get embed keys as a separate array to enable deleting keys in map
			for ( const String id_dep : embeds.keySet() ) {
				const EmbedNode e = embeds.get ( id_dep );
				const Object p = e.parent != null ? e.parent : newMap();
				if ( ! ( p.isMap() ) )
					continue;
				const String pid = ( String ) ( ( Map<String, Object> ) p ).get ( "@id" );
				if ( Obj.equals ( id, pid ) ) {
					embeds.remove ( id_dep );
					removeDependents ( embeds, id_dep );
				}
			}
		}

		Map<String, Object> filterNodes ( FramingContext state, Map<String, Object> nodes,
		                                  Map<String, Object> frame )  {
			const Map<String, Object> rval = newMap();
			for ( const String id : nodes.keySet() ) {
				const Map<String, Object> element = ( Map<String, Object> ) nodes.get ( id );
				if ( element != null && filterNode ( state, element, frame ) )
					rval.put ( id, element );
			}
			return rval;
		}

		boolean filterNode ( FramingContext state, Map<String, Object> node, Map<String, Object> frame )  {
			Object types;// = frame.get ( "@type" );
			//		if ( types != null ) {
			if ( frame.get ( "@type", types ) ) {
				if ( !  types.isList()  ) throw JsonLdError ( Error.SYNTAX_ERROR, "frame @type must be an array" );
				Object nodeTypes = node.get ( "@type" );
				if ( nodeTypes == null ) nodeTypes = new ArrayList<Object>();
				else if ( ! ( nodeTypes.isList() ) ) throw JsonLdError ( Error.SYNTAX_ERROR, "node @type must be an array" );
				if (  types.list() .size() == 1 &&  types.list().get ( 0 ) .isMap()
				        && types.list().get ( 0 ).map().size() == 0 )
					return ! nodeTypes.list() .isEmpty();
				else {
					for ( const Object i : ( List<Object> ) nodeTypes )
						for ( const Object j : ( List<Object> ) types )
							if ( JsonLdUtils.deepCompare ( i, j ) ) return true;
					return false;
				}
			} else {
				for ( const String key : frame.keySet() )
					if ( "@id"s == ( key ) || !isKeyword ( key ) && ! ( node.containsKey ( key ) ) )
						return false;
				return true;
			}
		}

		/**
		    Adds framing output to the given parent.

		    @param state
		              the current framing state.
		    @param parent
		              the parent to add to.
		    @param property
		              the parent property.
		    @param output
		              the output to add.
		*/
private: static void addFrameOutput ( FramingContext & state, Object & parent, String & property,
		                                      Object & output ) {
			if ( parent.isMap() ) {
				List<Object> prop = ( List<Object> ) ( ( Map<String, Object> ) parent ).get ( property );
				if ( prop == null ) {
					prop = new ArrayList<Object>();
					( ( Map<String, Object> ) parent ).put ( property, prop );
				}
				prop.push_back ( output );
			} else
				parent.list().push_back ( output );
		}

		/**
		    Embeds values for the given subject and property into the given output
		    during the framing algorithm.

		    @param state
		              the current framing state.
		    @param element
		              the subject.
		    @param property
		              the property.
		    @param output
		              the output.
		*/
		void embedValues ( FramingContext & state, Map<String, Object>& element, String & property, Object & output ) {
			// embed subject properties in output
			List<Object> objects = element.get ( property ).list();
			for ( Object o : objects ) {
				// handle subject reference
				if ( JsonLdUtils.isNodeReference ( o ) ) {
					const String sid = ( String ) ( ( Map<String, Object> ) o ).get ( "@id" );

					// embed full subject if isn't already embedded
					if ( !state.embeds.containsKey ( sid ) ) {
						// add embed
						const EmbedNode embed = new EmbedNode();
						embed.parent = output;
						embed.property = property;
						state.embeds.put ( sid, embed );

						// recurse into subject
						o = newMap();
						Map<String, Object> s = ( Map<String, Object> ) this.nodeMap.get ( sid );
						if ( s == null )
							s = newMap ( "@id", sid );
						for ( const String prop : s.keySet() ) {
							// copy keywords
							if ( isKeyword ( prop ) ) {
								( ( Map<String, Object> ) o ).put ( prop, JsonLdUtils.clone ( s.get ( prop ) ) );
								continue;
							}
							embedValues ( state, s, prop, o );
						}
					}
					addFrameOutput ( state, output, property, o );
				}
				// copy non-subject value
				else
					addFrameOutput ( state, output, property, JsonLdUtils.clone ( o ) );
			}
		}

private:
		class UsagesNode {
		public: UsagesNode ( NodeMapNode& node_, String& property_, Map<String, Object>& value_ ) :
				node ( node ),
				property ( property ),
				value ( value ) {
			}

			NodeMapNode& node;// = null;
			String& property;// = null;
			Map<String, Object>& value;// = null;
		};

		class NodeMapNode : public LinkedHashMap<String, Object> {
		public:
			List<UsagesNode> usages;

			NodeMapNode ( String id ) : LinkedHashMap<String, Object>() {
				put ( "@id", id );
			}

			// helper fucntion for 4.3.3
			boolean isWellFormedListNode() {
				if ( usages.size() != 1 )
					return false;
				int keys = 0;
				if ( containsKey ( RDF_FIRST ) ) {
					keys++;
					if ( ! ( get ( RDF_FIRST ).isList() &&  get ( RDF_FIRST ).list() .size() == 1 ) )
						return false;
				}
				if ( containsKey ( RDF_REST ) ) {
					keys++;
					if ( ! ( get ( RDF_REST ).isList() &&  get ( RDF_REST ).list().size() == 1 ) )
						return false;
				}
				if ( containsKey ( "@type" ) ) {
					keys++;
					if ( ! ( get ( "@type" ).isList() &&  get ( "@type" ).list( ).size() == 1 )
					        && RDF_LIST.equals (  get ( "@type" ).list( ).get ( 0 ) ) )
						return false;
				}
				// TODO: SPEC: 4.3.3 has no mention of @id
				if ( containsKey ( "@id" ) ) keys++;
				return keys >= size();
			}

			// return this node without the usages variable
		public:
			Map<String, Object> serialize() {
				return new LinkedHashMap<String, Object> ( this );
			}
		};

		/**
		    Converts RDF statements into JSON-LD.

		    @param dataset
		              the RDF statements.
		    @return A list of JSON-LD objects found in the given dataset.
		    @
		               If there was an error during conversion from RDF to JSON-LD.
		*/
public:
		List<Object> fromRDF ( const RDFDataset dataset )  {
			// 1)
			const Map<String, NodeMapNode> defaultGraph = new LinkedHashMap<String, NodeMapNode>();
			// 2)
			const Map<String, Map<String, NodeMapNode>> graphMap = new LinkedHashMap<String, Map<String, NodeMapNode>>();
			graphMap.put ( "@default", defaultGraph );

			// 3/3.1)
			for ( const String name : dataset.graphNames() ) {
				const List<RDFDataset.Quad> graph = dataset.getQuads ( name );
				// 3.2+3.4)
				Map<String, NodeMapNode> nodeMap;
				if ( !graphMap.containsKey ( name ) ) {
					nodeMap = LinkedHashMap<String, NodeMapNode>();
					graphMap.put ( name, nodeMap );
				} else
					nodeMap = graphMap.get ( name );

				// 3.3)
				if ( !"@default"s == ( name ) && !Obj.contains ( defaultGraph, name ) )
					defaultGraph.put ( name, NodeMapNode ( name ) );

				// 3.5)
				for ( const RDFDataset.Quad triple : graph ) {
					const String subject = triple.getSubject().getValue();
					const String predicate = triple.getPredicate().getValue();
					const RDFDataset.Node object = triple.getObject();

					// 3.5.1+3.5.2)
					NodeMapNode node;
					if ( !nodeMap.containsKey ( subject ) ) {
						node = new NodeMapNode ( subject );
						nodeMap.put ( subject, node );
					} else
						node = nodeMap.get ( subject );

					// 3.5.3)
					if ( ( object.isIRI() || object.isBlankNode() )
					        && !nodeMap.containsKey ( object.getValue() ) )
						nodeMap.put ( object.getValue(), new NodeMapNode ( object.getValue() ) );

					// 3.5.4)
					if ( RDF_TYPE.equals ( predicate ) && ( object.isIRI() || object.isBlankNode() )
					        && !opts.getUseRdfType() ) {
						JsonLdUtils.mergeValue ( node, "@type", object.getValue() );
						continue;
					}

					// 3.5.5)
					const Map<String, Object> value = object.toObject ( opts.getUseNativeTypes() );

					// 3.5.6+7)
					JsonLdUtils.mergeValue ( node, predicate, value );

					// 3.5.8)
					if ( object.isBlankNode() || object.isIRI() ) {
						// 3.5.8.1-3)
						nodeMap.get ( object.getValue() ).usages
						.add ( new UsagesNode ( node, predicate, value ) );
					}
				}
			}

			// 4)
			for ( const String name : graphMap.keySet() ) {
				const Map<String, NodeMapNode> graph = graphMap.get ( name );

				// 4.1)
				if ( !graph.containsKey ( RDF_NIL ) )
					continue;

				// 4.2)
				const NodeMapNode nil = graph.get ( RDF_NIL );
				// 4.3)
				for ( const UsagesNode usage : nil.usages ) {
					// 4.3.1)
					NodeMapNode node = usage.node;
					String property = usage.property;
					Map<String, Object> head = usage.value;
					// 4.3.2)
					List<Object> list;
					List<String> listNodes;
					while ( RDF_REST.equals ( property ) && node.isWellFormedListNode() ) { // 4.3.3)
						list.add ( ( ( List<Object> ) node.get ( RDF_FIRST ) ).get ( 0 ) ); // 4.3.3.1)
						listNodes.add ( ( String ) node.get ( "@id" ) ); // 4.3.3.2)
						const UsagesNode nodeUsage = node.usages.get ( 0 ); // 4.3.3.3)
						node = nodeUsage.node; // 4.3.3.4)
						property = nodeUsage.property;
						head = nodeUsage.value;
						// 4.3.3.5)
						if ( !JsonLdUtils.isBlankNode ( node ) )
							break;
					}
					if ( RDF_FIRST.equals ( property ) ) { // 4.3.4)
						if ( RDF_NIL.equals ( node.get ( "@id" ) ) ) continue; // 4.3.4.1)
						const String headId = ( String ) head.get ( "@id" ); // 4.3.4.3)
						head = ( Map<String, Object> ) ( ( List<Object> ) graph.get ( headId ).get ( RDF_REST ) ) .get ( 0 );// 4.3.4.4-5)
						list.remove ( list.size() - 1 ); // 4.3.4.6)
						listNodes.remove ( listNodes.size() - 1 );
					}
					head.remove ( "@id" ); // 4.3.5
					Collections.reverse ( list ); // 4.3.6
					head.put ( "@list", list ); // 4.3.7
					for ( const String nodeId : listNodes ) graph.remove ( nodeId ); // 4.3.8
				}
			}

			List<Object> result; // 5
			//		const List<String> ids = new ArrayList<String> ( defaultGraph.keySet() ); // 6
			//		Collections.sort ( ids );
			//		for ( const String subject : ids ) {
			for ( auto x : defaultGraph() ) {
				String subject = x.first;
				const NodeMapNode node = defaultGraph.get ( subject );
				if ( graphMap.containsKey ( subject ) ) { // 6.1
					node.put ( "@graph", new ArrayList<Object>() ); // 6.1.1
					// 6.1.2)
					//				List<String> keys( graphMap.get ( subject ).keySet() );
					//				Collections.sort ( keys );
					//				for ( const String s : keys ) {
					for ( auto y : graphMap.get ( subject ) ) {
						Strings s = y.first;
						NodeMapNode n = y.second;//graphMap.get ( subject ).get ( s );
						if ( n.size() == 1 && n.containsKey ( "@id" ) ) continue;
						node.get ( "@graph" ).list().push_back ( n.serialize() );
					}
				}
				// 6.2)
				if ( node.size() == 1 && node.containsKey ( "@id" ) )
					continue;
				result.add ( node.serialize() );
			}

			return result;
		}

		/***
		    ____ _ _ ____ ____ _____ _ _ _ _ _ / ___|___ _ ____ _____ _ __| |_ | |_
		    ___ | _ \| _ \| ___| / \ | | __ _ ___ _ __(_) |_| |__ _ __ ___ | | / _ \|
		    '_ \ \ / / _ \ '__| __| | __/ _ \ | |_) | | | | |_ / _ \ | |/ _` |/ _ \|
		    '__| | __| '_ \| '_ ` _ \ | |__| (_) | | | \ V / __/ | | |_ | || (_) | |
		    _ <| |_| | _| / ___ \| | (_| | (_) | | | | |_| | | | | | | | |
		    \____\___/|_| |_|\_/ \___|_| \__| \__\___/ |_| \_\____/|_| /_/ \_\_|\__,
		    |\___/|_| |_|\__|_| |_|_| |_| |_| |___/
		*/

		/**
		    Adds RDF triples for each graph in the current node map to an RDF
		    dataset.

		    @return the RDF dataset.
		    @
		               If there was an error converting from JSON-LD to RDF.
		*/
		RDFDataset toRDF()  {
			// TODO: make the default generateNodeMap call (i.e. without a
			// graphName) create and return the nodeMap
			const Map<String, Object> nodeMap;
			nodeMap.put ( "@default", newMap() );
			generateNodeMap ( this.value, nodeMap );

			const RDFDataset dataset = new RDFDataset ( this );

			for ( const String graphName : nodeMap.keySet() ) {
				if ( JsonLdUtils.isRelativeIri ( graphName ) ) continue; // 4.1
				dataset.graphToRDF ( graphName, nodeMap.get ( graphName ).obj() );
			}

			return dataset;
		}

		/***
		    _ _ _ _ _ _ _ _ _ _ _ | \ | | ___ _ __ _ __ ___ __ _| (_)______ _| |_(_)
		    ___ _ __ / \ | | __ _ ___ _ __(_) |_| |__ _ __ ___ | \| |/ _ \| '__| '_ `
		    _ \ / _` | | |_ / _` | __| |/ _ \| '_ \ / _ \ | |/ _` |/ _ \| '__| | __|
		    '_ \| '_ ` _ \ | |\ | (_) | | | | | | | | (_| | | |/ / (_| | |_| | (_) |
		    | | | / ___ \| | (_| | (_) | | | | |_| | | | | | | | | |_| \_|\___/|_|
		    |_| |_| |_|\__,_|_|_/___\__,_|\__|_|\___/|_| |_| /_/ \_\_|\__, |\___/|_|
		    |_|\__|_| |_|_| |_| |_| |___/
		*/

		/**
		    Performs RDF normalization on the given JSON-LD input.

		    @param dataset
		              the expanded JSON-LD object to normalize.
		    @return The normalized JSON-LD object
		    @
		               If there was an error while normalizing.
		*/
		Object normalize ( Map<String, Object> dataset )  {
			// create quads and map bnodes to their associated quads
			List<Object> quads;
			Map<String, Object> bnodes;
			for ( auto x : dataset ) {
				String graphName = x.first;
				//List<Map<String, Object>> triples = ( List<Map<String, Object>> ) dataset .get ( graphName );
				List</*Map<String, */Object> triples = dataset .get ( graphName ).list();
				if ( "@default"s == graphName ) graphName = null;
				//for ( const Map<String, Object> quad : triples ) {
				for ( auto y : triples ) {
					Map<String, Object> quad = y->obj();
					if ( graphName.size() ) {
						Map<String, Object> tmp;
						tmp.put ( "type", graphName.indexOf ( "_:" ) ? "IRI" : "blank node" );
						tmp.put ( "value", graphName );
						quad.put ( "name", tmp );
					}
					quads.add ( quad );

					//				const String[] attrs = { "subject", "object", "name" };
					for ( String attr : {
					            "subject", "object", "name"
					        } ) {
						if ( quad.containsKey ( attr ) && "blank node"s == quad.get ( attr ).obj( ) .get ( "type" ) ) {
							String id = quad.get ( attr ).obj( ) .get ( "value" ).str();
							if ( !bnodes.containsKey ( id ) ) {
								bnodes.put ( id, new LinkedHashMap<String, List<Object>>() {
									{
										put ( "quads", new ArrayList<Object>() );
									}
								} );
							}
							bnodes.get ( id ).obj( ).get ( "quads" ) .list().push_back ( quad );
						}
					}
				}
			}

			// mapping complete, start canonical naming
			return NormalizeUtils ( quads, bnodes, UniqueNamer ( "_:c14n" ), opts ).hashBlankNodes ( bnodes );
			//		return normalizeUtils.hashBlankNodes ( bnodes.keySet() );
		}

	}
	;
