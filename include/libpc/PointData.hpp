/******************************************************************************
* Copyright (c) 2011, Michael P. Gerlek (mpg@flaxen.com)
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#ifndef INCLUDED_POINTDATA_HPP
#define INCLUDED_POINTDATA_HPP

#include <boost/cstdint.hpp>

#include "libpc/export.hpp"
#include "libpc/SchemaLayout.hpp"

namespace libpc
{

// a PointData object is just an untyped array of N bytes,
// where N is (the size of the given Schema * the number of points)
//
// That is, a PointData represents the underlying data for one or more points.
//
// A PointData object has an associated Schema object.
//
// Many of the methods take a first parameter "index", to specify which point in the
// collection is to be operated upon.  The point index is a uint32; you can't read
// more than 4 billion points at a time.
class LIBPC_DLL PointData
{
public:
    // note that when we make a PointData object all the fields are initialized to inactive,
    // regardless of what the passed-in schema says -- this is because the field object
    // represents the state within the owning object, which in this case is a completely
    // empty buffer (similarly, all the points in the buffer are marked "invalid")
    PointData(const SchemaLayout&, boost::uint32_t numPoints);
    ~PointData();

    // number of points in this buffer
    boost::uint32_t getNumPoints() const;

    // number of points in this buffer that have legit data; initially will be zero,
    // and after a read() call it will be in the range 0 to getNumPoints()-1
    boost::uint32_t getNumValidPoints();

    // schema (number and kinds of fields) for a point in this buffer
    const SchemaLayout& getSchemaLayout() const
    {
        return m_schemaLayout;
    }

    // convenience function
    const Schema& getSchema() const
    {
        return m_schemaLayout.getSchema();
    }

    // "valid" means the data for the point can be used; if invalid, the point should
    // be ignored or skipped.  (This is done for efficiency; we don't want to have to
    // modify the buffer's size just to "delete" a point.)
    bool isValid(std::size_t pointIndex) const;
    void setValid(std::size_t pointIndex, bool value=true);

    // accessors to a particular field of a particular point in this buffer
    template<class T> T getField(std::size_t pointIndex, std::size_t fieldIndex) const;
    template<class T> void setField(std::size_t pointIndex, std::size_t fieldIndex, T value);

    // bulk copy all the fields from the given point into this object
    // NOTE: this is only legal if the src and dest schemas are exactly the same
    // (later, this will be implemented properly, to handle the general cases slowly and the best case quickly)
    void copyPointFast(std::size_t destPointIndex, std::size_t srcPointIndex, const PointData& srcPointData);
    
    // same as above, but copies N points
    void copyPointsFast(std::size_t destPointIndex, std::size_t srcPointIndex, const PointData& srcPointData, std::size_t numPoints);

private:
    // access to the raw memory
    boost::uint8_t* getData(std::size_t pointIndex) const;

    SchemaLayout m_schemaLayout;
    boost::uint8_t* m_data;
    std::size_t m_pointSize;
    boost::uint32_t m_numPoints;
    std::vector<bool> m_isValid; // one bool for each point

    PointData(const PointData&); // not implemented
    PointData& operator=(const PointData&); // not implemented
};


template <class T>
void PointData::setField(std::size_t pointIndex, std::size_t fieldIndex, T value)
{
    std::size_t offset = (pointIndex * m_pointSize) + m_schemaLayout.getDimensionLayout(fieldIndex).getByteOffset();
    assert(offset + sizeof(T) <= m_pointSize * m_numPoints);
    boost::uint8_t* p = m_data + offset;

    *(T*)p = value;
}


template <class T>
T PointData::getField(std::size_t pointIndex, std::size_t fieldIndex) const
{
    std::size_t offset = (pointIndex * m_pointSize) + m_schemaLayout.getDimensionLayout(fieldIndex).getByteOffset();
    assert(offset + sizeof(T) <= m_pointSize * m_numPoints);
    boost::uint8_t* p = m_data + offset;

    return *(T*)p;
}


LIBPC_DLL std::ostream& operator<<(std::ostream& ostr, const PointData&);


} // namespace libpc

#endif
