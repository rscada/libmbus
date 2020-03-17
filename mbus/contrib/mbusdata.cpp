#include "mbusdata.hpp"

#include "fmt/format.h"

namespace libmbus {

void MbusData::import_frame_data(mbus_frame_data &reply_data, MbusData *info)
{
    if (reply_data.type == MBUS_DATA_TYPE_FIXED) {
        mbus_data_fixed *data_to_read = &reply_data.data_fix;

        info->slave_information_.id_ = mbus_data_bcd_decode(data_to_read->id_bcd, 4);
        info->slave_information_.AccessNumber = data_to_read->tx_cnt;
        info->slave_information_.Status = fmt::format("{%.2X}", data_to_read->status);
    }

    if (reply_data.type == MBUS_DATA_TYPE_VARIABLE) {
        mbus_data_variable *data_to_read = &reply_data.data_var;
        mbus_data_record *record{};
        int i{};

        for (record = data_to_read->record, i = 0; record;
             record = static_cast<mbus_data_record *>(record->next), i++) {
            DataRecord data_record{};
            int frame_cnt = -1;
            int record_cnt = i;
            data_record.id_ = record_cnt;
            data_record.frame_ = frame_cnt;

            std::string str_encoded;
            const unsigned int MAX_XML_ENCODED_STRING_LENGTH = 768;

            std::string unit(mbus_data_record_unit(record));
            mbus_str_xml_encode(reinterpret_cast<unsigned char *>(str_encoded.data()),
                                reinterpret_cast<const unsigned char *>(unit.data()),
                                MAX_XML_ENCODED_STRING_LENGTH);

            data_record.unit_ = str_encoded;

            std::string value;
            value = mbus_data_record_value(record);

            mbus_str_xml_encode(reinterpret_cast<unsigned char *>(str_encoded.data()),
                                reinterpret_cast<const unsigned char *>(value.data()),
                                MAX_XML_ENCODED_STRING_LENGTH);

            data_record.value_ = str_encoded;
            info->data_records_.push_back(data_record);
        }
    }
}

} // namespace libmbus
