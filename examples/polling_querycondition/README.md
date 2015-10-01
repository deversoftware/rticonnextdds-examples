# Example Code: Polling With Query Condition

   This either looks like the `WHERE` clause in SQL, such as `x < 34 OR y > 2`
   or it can be a string parameter that can be compared using a `MATCH`
   comparison such as `symbol MATCH 'IBM'`. This query can have parameters to
   it, represented with %0, %1, etc. These parameters can be changed at runtime.

2. Creating the *QueryCondition* object

   You use your *DataReader* to create a *QueryCondition* object, specifying the
   query that you want to run, and parameters to that query.


   You can call `read_w_condition()` or `take_w_condition()` to retrieve only
   the data that passes the query from the *DataReader's* cache.


   You can update the parameters to your query by calling
   `set_query_parameters`.

Note that a *QueryCondition* requires that the single quotes around a string
are _inside the query condition parameter_. For example:
```c
DDS_StringSeq queryParameters;

// DON'T FORGET THE SINGLE QUOTES INSIDE THE PARAMETER
query_condition = _reader->create_querycondition(DDS_ANY_SAMPLE_STATE,

// ...

// DON'T FORGET THE SINGLE QUOTES INSIDE THE PARAMETER