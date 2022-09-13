#include "object/delta_type_class.h"

namespace scs {

DeltaTypeClass::DeltaTypeClass()
    : valence()
{
    valence.tv.type(TypeclassValence::TV_FREE);
    valence.deleted_last = 0;
}

void
DeltaTypeClass::set_error()
{
    valence.tv.type(TypeclassValence::TV_ERROR);
    valence.deleted_last = 0;
}

bool
DeltaTypeClass::can_accept(StorageDelta const& d)
{
    switch (valence.tv.type()) {
        case TypeclassValence::TV_FREE:
            return true;
    //    case TypeclassValence::TV_DELETE_FIRST:
      //      return (d.type() == DeltaType::DELETE_FIRST)
        //           || (d.type() == DeltaType::DELETE_LAST);
        case TypeclassValence::TV_RAW_MEMORY_WRITE:
            if (d.type() == DeltaType::DELETE_LAST) {
                return true;
            }
            if (d.type() != DeltaType::RAW_MEMORY_WRITE) {
                return false;
            }
            return d.data() == valence.tv.data();
        case TypeclassValence::TV_NONNEGATIVE_INT64_SET:
            if (d.type() == DeltaType::DELETE_LAST) {
                return true;
            }
            if (d.type() != DeltaType::RAW_MEMORY_WRITE) {
                return false;
            }
            return d.set_add_nonnegative_int64().set_value
                   == valence.tv.set_value();
        case TypeclassValence::TV_ERROR:
            return false;
        default:
            throw std::runtime_error("invalid typeclass valence");
    }
}

void
DeltaTypeClass::add(StorageDelta const& d)
{
    switch (valence.tv.type()) {
        case TypeclassValence::TV_FREE: {
            switch (d.type()) {
       //         case DeltaType::DELETE_FIRST:
         //           valence.tv.type(TypeclassValence::TV_DELETE_FIRST);
           //         break;
                case DeltaType::DELETE_LAST:
                    valence.deleted_last = 1;
                    break;
                case DeltaType::RAW_MEMORY_WRITE:
                    valence.tv.type(TypeclassValence::TV_RAW_MEMORY_WRITE);
                    valence.tv.data() = d.data();
                    break;
                case DeltaType::NONNEGATIVE_INT64_SET_ADD:
                    valence.tv.type(TypeclassValence::TV_NONNEGATIVE_INT64_SET);
                    valence.tv.set_value()
                        = d.set_add_nonnegative_int64().set_value;
                    break;
            }
            break;
        }
   /*     case TypeclassValence::TV_DELETE_FIRST: {
            switch (d.type()) {
                case DeltaType::DELETE_FIRST:
                    break;
                case DeltaType::DELETE_LAST:
                    valence.deleted_last = 1;
                    break;
                default:
                    valence.tv.type(TypeclassValence::TV_ERROR);
                    break;
            }
        } break; */
        case TypeclassValence::TV_RAW_MEMORY_WRITE: {
            switch (d.type()) {
                case DeltaType::RAW_MEMORY_WRITE:
                    if (valence.tv.data() != d.data()) {
                        set_error();
                    }
                    break;
                case DeltaType::DELETE_LAST:
                    valence.deleted_last = 1;
                    break;
                default:
                    set_error();
                    break;
            }
        } break;
        case TypeclassValence::TV_NONNEGATIVE_INT64_SET: {
            switch (d.type()) {
                case DeltaType::NONNEGATIVE_INT64_SET_ADD:
                    if (valence.tv.set_value()
                        != d.set_add_nonnegative_int64().set_value) {
                        set_error();
                    }
                    break;
                case DeltaType::DELETE_LAST:
                    valence.deleted_last = 1;
                    break;
                default:
                    set_error();
                    break;
            }
        } break;
        case TypeclassValence::TV_ERROR:
            break;

        default:
            throw std::runtime_error("Unimplemented typeclass");
    }
}

void
DeltaTypeClass::overwrite_free_tc(const DeltaValence& other)
{
    uint32_t old_deleted_last = valence.deleted_last;
    valence = other;
    valence.deleted_last = old_deleted_last;
}

void
DeltaTypeClass::add(DeltaTypeClass const& dtc)
{
    if (dtc.valence.deleted_last > 0) {
        valence.deleted_last = 1;
    }

    switch (dtc.valence.tv.type()) {
        case TypeclassValence::TV_FREE:
            return;

      /*  case TypeclassValence::TV_DELETE_FIRST: {
            switch (valence.tv.type()) {
                case TypeclassValence::TV_FREE:
                    overwrite_free_tc(dtc.valence);
                    return;
                case TypeclassValence::TV_DELETE_FIRST:
                    return;
                default:
                    valence.tv.type(TypeclassValence::TV_ERROR);
                    return;
            }
        } */
        case TypeclassValence::TV_RAW_MEMORY_WRITE: {
            switch (valence.tv.type()) {
                case TypeclassValence::TV_FREE:
                    overwrite_free_tc(dtc.valence);
                    return;
                case TypeclassValence::TV_RAW_MEMORY_WRITE:
                    if (valence.tv.data() != dtc.valence.tv.data()) {
                        valence.tv.type(TypeclassValence::TV_ERROR);
                    }
                    return;
                default:
                    valence.tv.type(TypeclassValence::TV_ERROR);
                    return;
            }
        }
        case TypeclassValence::TV_NONNEGATIVE_INT64_SET: {
            switch (valence.tv.type()) {
                case TypeclassValence::TV_FREE:
                    overwrite_free_tc(dtc.valence);
                    return;
                case TypeclassValence::TV_NONNEGATIVE_INT64_SET:
                    if (valence.tv.set_value() != dtc.valence.tv.set_value()) {
                        set_error();
                    }
                    return;
                default:
                    set_error();
                    return;
            }
        }
        case TypeclassValence::TV_ERROR: {
            set_error();
            return;
        }
        default:
            throw std::runtime_error("unimplemented case");
    }
}

#if 0

DeltaTypeClass::DeltaTypeClass(StorageDelta const& origin_delta, DeltaPriority const& priority)
	: priority(priority)
	, base_delta(origin_delta)
	{}

DeltaTypeClass::DeltaTypeClass(StorageDelta const& origin_delta)
	: priority(std::nullopt)
	, base_delta(origin_delta)
	{}

bool
DeltaTypeClass::is_lower_rank_than(StorageDelta const& new_delta, DeltaPriority const& new_priority) const
{
	if (!priority)
	{
		throw std::runtime_error("cannot compare rank without self priority");
	}

	if (new_delta.type() == base_delta.type())
	{
		return (*priority) < new_priority;
	}

	if (new_delta.type() == DeltaType::DELETE_LAST) // and base_delta.type() != DELETE_LAST
	{
		return false;
	}
	if (base_delta.type() == DeltaType::DELETE_FIRST) // and new_delta.type() != DELETE_FIRST
	{
		return true;
	}
	return (*priority) < new_priority;
}

bool 
DeltaTypeClass::is_lower_rank_than(DeltaTypeClass const& other) const
{
	if (!other.priority)
	{
		throw std::runtime_error("cannot compare rank without other priority");
	}
	return is_lower_rank_than(other.base_delta, *other.priority);
}


bool 
DeltaTypeClass::accepts(StorageDelta const& delta) const
{
	// always accept delete_last
	if (delta.type() == DeltaType::DELETE_LAST)
	{
		return true;
	}

	/*
	//redundant 
	if (base_delta.type() == DeltaType::DELETE_FIRST)
	{
		return delta.type() == DeltaType::DELETE_FIRST;
	} */

	if (base_delta.type() != delta.type()) {
		return false;
	}

	if (base_delta.type() == DeltaType::RAW_MEMORY_WRITE)
	{
		return base_delta.data() == delta.data();
	}

	if (base_delta.type() == DeltaType::NONNEGATIVE_INT64_SET_ADD)
	{
		return base_delta.set_add_nonnegative_int64().set_value
			== delta.set_add_nonnegative_int64().set_value;
	}

	throw std::runtime_error("unhandled case in DeltaTypeClass::accepts");
}

#endif

} // namespace scs
