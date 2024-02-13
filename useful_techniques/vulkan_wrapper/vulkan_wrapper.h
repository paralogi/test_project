#pragma once

#include <vulkan/vulkan.hpp>

/**
 *
 */
template< typename SourceTag >
struct BufferWriter {
    char* buffer_data;
    std::size_t buffer_size;

    static BufferWriter create( ValueType& value ) {

        BufferWriter writer;
        writer.buffer_data = ( char* ) &value;
        writer.buffer_size = sizeof( ValueType );
        return writer;
    }

    static BufferWriter create( std::vector< ValueType >& buffer ) {

        BufferWriter writer;
        writer.buffer_data = ( char* ) buffer.data();
        writer.buffer_size = buffer.size() * sizeof( ValueType );
        return writer;
    }

    std::size_t offset() {

        return 0;
    }

    std::size_t size() {

        return buffer_size;
    }

    void write( char* data ) {

        std::copy_n( buffer_data, buffer_size, data );
    }

    void read( const char* data ) {

        std::copy_n( data, buffer_size, buffer_data );
    }
};

/**
 *
 */
class VulkanWrapper {
public:
    void initialize();

    uint32_t& deviceIndex();
    uint32_t& queueIndex();
    uint32_t& workgroupSize();
    std::size_t& workmemSize();

    const vk::PhysicalDeviceProperties& deviceProperties() const;
    const vk::PhysicalDeviceVulkan11Properties& devicePropertiesExt11() const;
    const vk::PhysicalDeviceMemoryProperties& deviceMemoryProperties() const;
    const vk::PhysicalDeviceFeatures& deviceFeatures() const;
    const vk::PhysicalDeviceVulkan11Features& deviceFeaturesExt11() const;
    const vk::PhysicalDeviceVulkan12Features& deviceFeaturesExt12() const;
    const vk::PhysicalDeviceShaderAtomicFloatFeaturesEXT& deviceFeaturesAtomicFloat() const;
    const vk::PhysicalDeviceLimits& deviceLimits() const;

    bool computeDouble() const;

    std::size_t memorySize( vk::MemoryPropertyFlags required = vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::MemoryPropertyFlags ignored = vk::MemoryPropertyFlagBits::eHostVisible ) const;

    vk::UniqueCommandBuffer commandBuffer( vk::CommandBufferAllocateInfo& command_buffer_info );

    vk::UniqueBuffer memoryBuffer( vk::BufferCreateInfo& buffer_info );

    vk::UniqueDeviceMemory deviceMemory( vk::Buffer buffer,
            vk::MemoryPropertyFlags required = vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::MemoryPropertyFlags ignored = vk::MemoryPropertyFlagBits::eHostVisible );

    vk::UniqueDeviceMemory deviceMemory( const vk::ArrayProxy< vk::Buffer >& buffers,
            vk::MemoryPropertyFlags required = vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::MemoryPropertyFlags ignored = vk::MemoryPropertyFlagBits::eHostVisible );

    vk::UniqueDescriptorSetLayout descriptorSetLayout(
            vk::DescriptorSetLayoutCreateInfo& descriptor_set_layout_info );

    vk::UniqueDescriptorSet descriptorSet( vk::DescriptorSetAllocateInfo& descriptor_set_info );

    void updateDescriptorSets( const vk::ArrayProxy< const vk::WriteDescriptorSet >& write_descriptor_sets );

    vk::UniqueShaderModule shaderModule( vk::ShaderModuleCreateInfo& shader_module_info );

    vk::UniquePipelineLayout pipelineLayout( vk::PipelineLayoutCreateInfo& pipeline_layout_info );

    vk::UniquePipeline computePipeline( vk::PipelineCache pipeline_cache,
            vk::ComputePipelineCreateInfo& compute_pipeline_info );

    void submitPipeline( vk::SubmitInfo& submit_info, uint64_t timeout = 10'000'000'000 );

    char* mmap( vk::DeviceMemory memory, vk::DeviceSize offset, vk::DeviceSize size );

    void unmap( vk::DeviceMemory memory );

    void copy( vk::Buffer buffer_src, vk::Buffer buffer_dst, const vk::BufferCopy& copy );

    void write( vk::DeviceMemory memory, BufferWriter&& writer );

    void write( vk::Buffer buffer, vk::BufferUsageFlagBits flags, BufferWriter&& writer );

    void read( vk::DeviceMemory memory, BufferWriter&& writer );

    void read( vk::Buffer buffer, vk::BufferUsageFlagBits flags, BufferWriter&& writer );

private:
    void createInstance();
    void createDevice();

    static VkBool32 debugUtilsMessengerCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
            VkDebugUtilsMessageTypeFlagsEXT message_types,
            const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
            void* /*user_data*/ );

private:
    vk::DynamicLoader m_loader;
    vk::UniqueInstance m_instance;
    vk::UniqueDebugUtilsMessengerEXT m_debug_utils;

    vk::PhysicalDeviceProperties2 m_device_properties2;
    vk::PhysicalDeviceVulkan11Properties m_device_properties_ext11;
    vk::PhysicalDeviceMemoryProperties m_memory_properties;
    vk::PhysicalDeviceFeatures2 m_device_features2;
    vk::PhysicalDeviceVulkan11Features m_device_features_ext11;
    vk::PhysicalDeviceVulkan12Features m_device_features_ext12;
    vk::PhysicalDeviceShaderAtomicFloatFeaturesEXT m_device_features_atomic_float;

    vk::PhysicalDevice m_physical_device;
    uint32_t m_device_index;
    uint32_t m_queue_index;
    uint32_t m_workgroup_size;
    std::size_t m_workmem_size;

    vk::UniqueDevice m_device;
    vk::UniqueDescriptorPool m_descriptor_pool;
    vk::UniqueCommandPool m_command_pool;
    vk::UniqueCommandBuffer m_update_buffer;
    vk::Queue m_queue;
};

/**
 *
 */
void VulkanWrapper::write( vk::DeviceMemory memory, BufferWriter&& writer ) {

    writer.write( mmap( memory, writer.offset(), writer.size() ) );
    unmap( memory );
}

/**
 *
 */
void VulkanWrapper::write( vk::Buffer buffer, vk::BufferUsageFlagBits flags, BufferWriter&& writer ) {

    vk::BufferCreateInfo staging_buffer_info;
    staging_buffer_info.size = writer.size();
    staging_buffer_info.usage = flags | vk::BufferUsageFlagBits::eTransferSrc;
    vk::UniqueBuffer staging_buffer = memoryBuffer( staging_buffer_info );
    vk::UniqueDeviceMemory staging_memory = deviceMemory( *staging_buffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, {} );

    writer.write( mmap( *staging_memory, 0, writer.size() ) );
    unmap( *staging_memory );

    copy( *staging_buffer, buffer, vk::BufferCopy( 0, writer.offset(), writer.size() ) );
}

/**
 *
 */
void VulkanWrapper::read( vk::DeviceMemory memory, BufferWriter&& writer ) {

    writer.read( mmap( memory, writer.offset(), writer.size() ) );
    unmap( memory );
}

/**
 *
 */
void VulkanWrapper::read( vk::Buffer buffer, vk::BufferUsageFlagBits flags, BufferWriter&& writer ) {

    vk::BufferCreateInfo staging_buffer_info;
    staging_buffer_info.size = writer.size();
    staging_buffer_info.usage = flags | vk::BufferUsageFlagBits::eTransferDst;
    vk::UniqueBuffer staging_buffer = memoryBuffer( staging_buffer_info );
    vk::UniqueDeviceMemory staging_memory = deviceMemory( *staging_buffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, {} );

    copy( buffer, *staging_buffer, vk::BufferCopy( writer.offset(), 0, writer.size() ) );

    writer.read( mmap( *staging_memory, 0, writer.size() ) );
    unmap( *staging_memory );
}
