# Currently working on README and documentation

# The ODF data format

The structure of an ODF document is very similar to the structure used in the JSON format, though it aims to be more memory efficient, while still having acceptable parsing speeds.

## Key idea

The main problem that should be solved by this new data format is the large overhead data in json arrays of a fixed type:
```json
{
  "objects": [
    {
      "id": 123,
      "value": 0.54,
      "registeredSince": 1999
    },
    ...
  ]
}
```
All properties of the objects (in this example 'id', 'value' and 'registeredSince') must be repeated on the definition of every object.

The Data inside of a document is organized into elements. Each `Element` contains 1 piece of data or a collection of other elements.  
An Element is splitted into a `Header` and a `Body`. While the `Body` contains the actual data of the element, the `Header` contains a list of specifiers to specify how the body is interpreted. Although every `Element` needs both a `Header` and a `Body`, they can be seperated or shared over multiple Elements.
