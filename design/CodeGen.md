# Ideas for code generation data structures

* Module contents
  * Functions
    * LinkName
  * TypeInfos
    * Class LinkName
    * Dispatch
  * InterfaceInfos
    * Interface LinkName
    * Reference to TypeInfo
    * Dispatch
  * Globals
    * LinkName


    source::Location _location;
    std::vector<TypeVar*> _typeVars;

* OutputSym
  * OutputClassInfoSym
    * td: TypeDefn
    * dispatch: DispatchTable
  * OutputInterfaceInfoSym
    * td: TypeDefn
    * dispatch: DispatchTable
  * OutputGlobalVarSym
    * vd: ValueDefn


FlexAlloc needs to work as follows:

  new() {
    self = super(size);
    // etc...
  }
