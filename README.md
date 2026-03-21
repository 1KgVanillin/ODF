# Currently working on README and documentation. Although the project should be functioning already, it is still under development.

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
To solve this problem, every object is splitted into a header, that contains information about the data (For example that "objects" is a list of objects, which all have the properties 'id', 'value' and 'registeredSince') and after than the actual data body. The header can then be reused over multiple bodies.

## Document structure and internal functionality

The Data inside of a document is organized into elements. Each `Element` contains 1 piece of data or a collection of other elements.  
An Element is splitted into a `Header` and a `Body`. While the `Body` contains the actual data of the element, the `Header` contains a list of specifiers to specify how the body is interpreted. Although every `Element` needs both a `Header` and a `Body`, they can be seperated or shared over multiple Elements.

### Spicifiers

Below is a list of all specifiers that can occur in a Header:

#### TypeSpecifier

The TypeSpecifier contains the type of the body. It is required to occur at least once in every header and is always the first specifier in it.  
Possible types are 8, 16, 32 and 64 bit precision signed and unsigned integers, 32 and 64 bit floating points, as well as a `List` and `Object` types and a few virtual control types that are used to manage other elements and types. More on those later.  
Objects and Lists are `container types`. `Container types` contain other Elements as childs and hold no other data apart from that. They are available in a `fixed` and a `mixed` version. In The `fixed` version, every child object shares the same header, like in the example scenario above.

#### SizeSpecifier

### Secondary SizeSpecifier

#### ObjectSpecifier


