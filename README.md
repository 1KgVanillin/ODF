# Currently working on README and documentation

# The ODF data format

The structure of an ODF document is very similar to the structure used in the JSON format, though it aims to be more memory efficient, while still having acceptable parsing speeds.

The Data inside of a document is organized into elements. Each `Element` contains 1 piece of data or a collection of other elements.  
An Element is splitted into a **Header** and a **Body**.
