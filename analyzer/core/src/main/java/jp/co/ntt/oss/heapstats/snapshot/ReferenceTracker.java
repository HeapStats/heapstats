/*
 * Copyright (C) 2015 Nippon Telegraph and Telephone Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package jp.co.ntt.oss.heapstats.snapshot;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.OptionalInt;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import jp.co.ntt.oss.heapstats.container.snapshot.ChildObjectData;
import jp.co.ntt.oss.heapstats.container.snapshot.ObjectData;

/**
 * This class tracks object references.
 * 
 * @author Yasumasa Suenaga
 */
public class ReferenceTracker {
    
    private final Map<Long, ObjectData> snapShot;
    
    private final OptionalInt rankLevel;
    
    private final Optional<Predicate<? super ObjectData>> filter;
    
    /**
     * Constructor of ReferenceTracker.
     * 
     * @param snapShot SnapShot to use.
     * @param rankLevel Ranking level. If this value is empty, ReferenceTracker returns all reference list.
     * @param filter Class filter. If this value is empty, ReferenceTracker returns all reference list.
     */
    public ReferenceTracker(Map<Long, ObjectData> snapShot, OptionalInt rankLevel, Optional<Predicate<? super ObjectData>> filter) {
        this.snapShot = snapShot;
        this.rankLevel = rankLevel;
        this.filter = filter;
    }
    
    /**
     * Build parent object data which is referenced from childTag.
     * 
     * @param parent Parent object.
     * @param childTag Object tag of child object.
     * @return Referenced ObjectData. If parent do not have childTag, this method will return empty.
     */
    private Optional<ObjectData> buildParentObjData(ObjectData parent, long childTag){
        Optional<ChildObjectData> child = parent.getReferenceList().stream()
                                                                   .filter(c -> (c.getTag() == childTag))
                                                                   .findAny();
        
        Optional<ObjectData> result = Optional.empty();
        
        if(child.isPresent()){
            ObjectData objData = parent.clone();
            objData.setCount(child.get().getInstances());
            objData.setTotalSize(child.get().getTotalSize());
            
            result = Optional.of(objData);
        }
        
        return result;
    }
    
    /**
     * Convert ChildObjectData to ObjectData.
     * 
     * @param child ChildObjectData instance to convert.
     * @return ObjectData.
     */
    private ObjectData childObjToObjData(ChildObjectData child){
        ObjectData result = snapShot.get(child.getTag()).clone();
        result.setCount(child.getInstances());
        result.setTotalSize(child.getTotalSize());
        
        return result;
    }
    
    /**
     * This method tracks object references which direction is parent.
     * 
     * @param startTag ObjectTag to start.
     * @param sortBySize Sort order. If this parameter is true, return list is
     *                    sorted by total size. Others, return list is sorted by
     *                    instance count.
     * @return List of parents.
     */
    public List<ObjectData> getParents(long startTag, boolean sortBySize){
        /* Pick up parents which have reference to target object */
        Stream<ObjectData> parentStream = snapShot.values().parallelStream()
                                                           .filter(o -> o.getReferenceList() != null);
        if(filter.isPresent()){
            parentStream = parentStream.filter(filter.get());
        }

        /* This comparator is reverse order. */
        Comparator<ObjectData> comparator = sortBySize ? Comparator.comparingLong(ObjectData::getTotalSize).reversed()
                                                       : Comparator.comparingLong(ObjectData::getCount).reversed();
        
        Stream<ObjectData> result = parentStream.map(o -> buildParentObjData(o, startTag))
                                                .filter(o -> o.isPresent())
                                                .map(o -> o.get())
                                                .sorted(comparator);
        
        if(rankLevel.isPresent()){
            result = result.limit(rankLevel.getAsInt());
        }
        
        return result.collect(Collectors.toList());
    }
    
    /**
     * This method tracks object references which direction is child.
     * 
     * @param startTag ObjectTag to start.
     * @param sortBySize Sort order. If this parameter is true, return list is
     *                    sorted by total size. Others, return list is sorted by
     *                    instance count.
     * @return List of children.
     */
    public List<ObjectData> getChildren(long startTag, boolean sortBySize){
        List<ChildObjectData> children = snapShot.get(startTag).getReferenceList();
        
        if(children == null){
            return new ArrayList<>();
        }
        
        /* Building ChildObjectData stream. */
        Stream<ChildObjectData> result = children.stream();
        
        if(filter.isPresent()){
            result = result.filter(c -> filter.get().test(snapShot.get(c.getTag())));
        }
        
        /* This comparator is reverse order. */
        Comparator<ChildObjectData> comparator = sortBySize ? Comparator.comparingLong(ChildObjectData::getTotalSize).reversed()
                                                            : Comparator.comparingLong(ChildObjectData::getInstances).reversed();
        result = result.sorted(comparator);
        
        if(rankLevel.isPresent()){
            result = result.limit(rankLevel.getAsInt());
        }
        
        return result.map(this::childObjToObjData)
                     .collect(Collectors.toList());
    }
    
}
