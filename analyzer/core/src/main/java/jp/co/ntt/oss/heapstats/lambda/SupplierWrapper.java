/*
 * Copyright (C) 2015 Yasumasa Suenaga
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
package jp.co.ntt.oss.heapstats.lambda;

import java.util.function.Supplier;

/**
 * Wrapper for Supplier interface which has check exception.
 * @author Yasumasa Suenaga
 */
public class SupplierWrapper<T> implements Supplier<T>{
    
    private final ExceptionalSupplier<T> supplier;

    public SupplierWrapper(ExceptionalSupplier<T> supplier) {
        this.supplier = supplier;
    }

    @Override
    public T get() {
        try{
            return supplier.get();
        }
        catch(Exception e){
            throw new RuntimeException(e);
        }
    }

}
