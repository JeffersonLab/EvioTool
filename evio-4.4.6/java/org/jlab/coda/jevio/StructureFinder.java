package org.jlab.coda.jevio;

import java.util.Enumeration;
import java.util.List;
import java.util.Vector;

/**
 * This is a set of convenient static methods used to find lists of structures
 * within an event, bank, segment, or tagsegment that match certain criteria. For the most
 * part it uses the <code>List<BaseStructure>&nbsp;getMatchingStructures(IEvioFilter)</code>
 * method on the provided <code>EvioEvent</code> object by constructing the
 * appropriate filter.
 *
 * @author heddle
 * @author timmer
 */
public class StructureFinder {

	
	/**
	 * Collect all the structures in an event that pass a filter.
	 * @param structure the event/bank/seg/tagseg being queried.
	 * @param filter the filter that must be passed. If <code>null</code>, this will return all the structures.
	 * @return a collection of all structures that are accepted by a filter for the provided event.
	 */
	public static List<BaseStructure> getMatchingStructures(BaseStructure structure, IEvioFilter filter) {
		if (structure == null) {
System.out.println("getMatchingStructures: returning null list");
			return null;
		}
		return structure.getMatchingStructures(filter);
	}
	
	/**
	 * Collect all the banks in an event that match a provided tag and number in their header.
     * Only Banks are returned, because only Banks have a number field.
     * @param structure the event/bank/seg/tagseg being queried.
	 * @param tag the tag to match.
	 * @param number the number to match.
	 * @return a collection of all Banks that are accepted by a filter for the provided event.
	 */
	public static List<BaseStructure> getMatchingBanks(BaseStructure structure, final int tag, final int number) {
		IEvioFilter filter = new IEvioFilter() {
			public boolean accept(StructureType type, IEvioStructure struct) {
				return (type == StructureType.BANK) &&
                       (tag == struct.getHeader().tag) &&
                       (number == struct.getHeader().number);
			}
		};
		return getMatchingStructures(structure, filter);
	}	
	
	/**
	 * Collect all the structures in an event that match a provided tag in their header.
     * @param structure the event/bank/seg/tagseg being queried.
	 * @param tag the tag to match.
	 * @return a collection of all structures that are accepted by a filter for the provided event.
	 */
	public static List<BaseStructure> getMatchingStructures(BaseStructure structure, final int tag) {
		IEvioFilter filter = new IEvioFilter() {
            public boolean accept(StructureType type, IEvioStructure struct) {
				return (tag == struct.getHeader().tag);
			}
		};
		return getMatchingStructures(structure, filter);
	}	

	/**
	 * Collect all the non-banks (i.e., Segments and TagSegments) in an event that match
     * a provided tag in their header. No Banks are returned.
     * @param structure the event/bank/seg/tagseg being queried.
	 * @param tag the tag to match.
	 * @return a collection of all non-bank structures that are accepted by a filter for the provided event.
	 */
	public static List<BaseStructure> getMatchingNonBanks(BaseStructure structure, final int tag) {
		IEvioFilter filter = new IEvioFilter() {
            public boolean accept(StructureType type, IEvioStructure struct) {
				return (type != StructureType.BANK) && (tag == struct.getHeader().tag);
			}
		};
		return getMatchingStructures(structure, filter);
	}	


    /**
     * Collect all structures in an event that match the given dictionary name.
     *
     * @param structure the event/bank/seg/tagseg being queried.
     * @param name       dictionary name of structures to be returned.
     * @param dictionary dictionary to be used; if null, an existing global dictionary will be used.
     * @return a list of BaseStructures that have the given name
     * @throws EvioException if no dictionary is defined
     */
    public static List<BaseStructure> getMatchingStructures(BaseStructure structure, String name,
                                                            INameProvider dictionary)
            throws EvioException {

        boolean useGlobalDictionary = false;

        if (dictionary == null) {
            if (!NameProvider.isProviderSet())  {
                throw new EvioException("Dictionary must be given as arg or defined globally in NameProvider");
            }
            else {
                useGlobalDictionary = true;
            }
        }

        // This IEvioFilter selects structures that match the given dictionary name
        class myEvioFilter implements IEvioFilter {
            String name;
            INameProvider dictionary;
            boolean useGlobalDictionary;

            myEvioFilter(boolean useGlobalDictionary, String name, INameProvider dictionary) {
                this.name = name;
                this.dictionary = dictionary;
                this.useGlobalDictionary = useGlobalDictionary;
            }

            public boolean accept(StructureType structureType, IEvioStructure struct) {
                String dictName;
                if (useGlobalDictionary) {
                    dictName = NameProvider.getName((BaseStructure)struct);
                }
                else {
                    dictName = dictionary.getName((BaseStructure)struct);
                }

                // If this structure matches the name, add it to the list
                return name.equals(dictName);
            }
        };

        myEvioFilter filter = new myEvioFilter(useGlobalDictionary, name, dictionary);
        return getMatchingStructures(structure, filter);
    }


    /**
     * Collect all structures in an event whose <b>parent</b> has the given dictionary name.
     *
     * @param structure  the event/bank/seg/tagseg being queried.
     * @param parentName dictionary name of parent of structures to be returned.
     * @param dictionary dictionary to be used; if null, an existing global dictionary will be used.
     * @return a list of BaseStructures whose parent has the given name
     * @throws EvioException if no dictionary is defined
     */
    public static List<BaseStructure> getMatchingParent(BaseStructure structure, String parentName,
                                                        INameProvider dictionary)
            throws EvioException {

        boolean useGlobalDictionary = false;

        if (dictionary == null) {
            if (!NameProvider.isProviderSet())  {
                throw new EvioException("Dictionary must be given as arg or defined globally in NameProvider");
            }
            else {
                useGlobalDictionary = true;
            }
        }

        // This IEvioFilter selects structures whose parent has the given dictionary name
        class myEvioFilter implements IEvioFilter {
            String name;
            INameProvider dictionary;
            boolean useGlobalDictionary;

            myEvioFilter(boolean useGlobalDictionary, String name, INameProvider dictionary) {
                this.name = name;
                this.dictionary = dictionary;
                this.useGlobalDictionary = useGlobalDictionary;
            }

            public boolean accept(StructureType structureType, IEvioStructure struct) {
                String dictName;

                BaseStructure parent = ((BaseStructure)struct).getParent();
                if (parent == null) {
                    return false;
                }

                if (useGlobalDictionary) {
                    dictName = NameProvider.getName(parent);
                }
                else {
                    dictName = dictionary.getName(parent);
                }

                // If this parent matches the name, add it to the list
                return name.equals(dictName);
            }
        };

        myEvioFilter filter = new myEvioFilter(useGlobalDictionary, parentName, dictionary);
        return getMatchingStructures(structure, filter);
    }


    /**
     * Collect all structures in an event who has a <b>child</b> with the given dictionary name.
     *
     * @param structure  the event/bank/seg/tagseg being queried.
     * @param childName  dictionary name of a child of structures to be returned.
     * @param dictionary dictionary to be used; if null, an existing global dictionary will be used.
     * @return a list of BaseStructures who has a child with the given name
     * @throws EvioException if no dictionary is defined
     */
    public static List<BaseStructure> getMatchingChild(BaseStructure structure, String childName,
                                                       INameProvider dictionary)
            throws EvioException {

        boolean useGlobalDictionary = false;

        if (dictionary == null) {
            if (!NameProvider.isProviderSet())  {
                throw new EvioException("Dictionary must be given as arg or defined globally in NameProvider");
            }
            else {
                useGlobalDictionary = true;
            }
        }

        // This IEvioFilter selects structures who have a child with the given dictionary name
        class myEvioFilter implements IEvioFilter {
            String name;
            INameProvider dictionary;
            boolean useGlobalDictionary;

            myEvioFilter(boolean useGlobalDictionary, String name, INameProvider dictionary) {
                this.name = name;
                this.dictionary = dictionary;
                this.useGlobalDictionary = useGlobalDictionary;
            }

            public boolean accept(StructureType structureType, IEvioStructure struct) {
                String dictName;

                Vector<BaseStructure> children = ((BaseStructure)struct).getChildren();
                if (children == null || children.size() < 1) {
                    return false;
                }

                BaseStructure bStruct;
                Enumeration<BaseStructure> enumeration = children.elements();
                while (enumeration.hasMoreElements()) {
                    bStruct = enumeration.nextElement();
                    if (useGlobalDictionary) {
                        dictName = NameProvider.getName(bStruct);
                    }
                    else {
                        dictName = dictionary.getName(bStruct);
                    }

                    if (name.equals(dictName)) {
                        // If this child matches the name, add it to the list
                        return true;
                    }
                }

                return false;
            }
        };

        myEvioFilter filter = new myEvioFilter(useGlobalDictionary, childName, dictionary);
        return getMatchingStructures(structure, filter);
    }


}
