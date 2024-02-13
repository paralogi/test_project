#include "vulkan.h"

#include <iostream>
#include <bitset>

#if !defined( VK_VERSION_1_2 )
#error "no vulkan 1.2 support"
#endif

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

/**
 *
 */
void Vulkan::initialize() {

    if ( !m_instance )
        createInstance();
    if ( !m_device )
        createDevice();
}

/**
 *
 */
uint32_t& Vulkan::deviceIndex() {

    return m_device_index;
}

/**
 *
 */
uint32_t& Vulkan::queueIndex() {

    return m_queue_index;
}

/**
 *
 */
uint32_t& Vulkan::workgroupSize() {

    return m_workgroup_size;
}

/**
 *
 */
std::size_t& Vulkan::workmemSize() {

    return m_workmem_size;
}

/**
 *
 */
const vk::PhysicalDeviceProperties& Vulkan::deviceProperties() const {

    return m_device_properties2.properties;
}

/**
 *
 */
const vk::PhysicalDeviceVulkan11Properties& Vulkan::devicePropertiesExt11() const {

    return m_device_properties_ext11;
}

/**
 *
 */
const vk::PhysicalDeviceMemoryProperties& Vulkan::deviceMemoryProperties() const {

    return m_memory_properties;
}

/**
 *
 */
const vk::PhysicalDeviceFeatures& Vulkan::deviceFeatures() const {

    return m_device_features2.features;
}

/**
 *
 */
const vk::PhysicalDeviceVulkan11Features& Vulkan::deviceFeaturesExt11() const {

    return m_device_features_ext11;
}

/**
 *
 */
const vk::PhysicalDeviceVulkan12Features& Vulkan::deviceFeaturesExt12() const {

    return m_device_features_ext12;
}

/**
 *
 */
const vk::PhysicalDeviceShaderAtomicFloatFeaturesEXT& Vulkan::deviceFeaturesAtomicFloat() const {

    return m_device_features_atomic_float;
}

/**
 *
 */
const vk::PhysicalDeviceLimits& Vulkan::deviceLimits() const {

    return m_device_properties2.properties.limits;
}


/**
 *
 */
bool Vulkan::computeDouble() const {

    return m_device_features_atomic_float.shaderBufferFloat64AtomicAdd == VK_TRUE;
}

/**
 *
 */
std::size_t Vulkan::memorySize( vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags ignored ) const {

    for ( uint32_t memory_id = 0; memory_id < m_memory_properties.memoryTypeCount; memory_id++ ) {
        auto& memory_type = m_memory_properties.memoryTypes[ memory_id ];
        if ( ( memory_type.propertyFlags & required ) == required &&
                ( memory_type.propertyFlags & ignored ) == vk::MemoryPropertyFlags() ) {
            auto& memory_heap = m_memory_properties.memoryHeaps[ memory_type.heapIndex ];
            return memory_heap.size;
        }
    }

    return 0;
}

/**
 *
 */
vk::UniqueCommandBuffer Vulkan::commandBuffer( vk::CommandBufferAllocateInfo& command_buffer_info ) {

    command_buffer_info.commandPool = *m_command_pool;
    command_buffer_info.commandBufferCount = 1;
    return std::move( m_device->allocateCommandBuffersUnique( command_buffer_info ).front() );
}

/**
 *
 */
vk::UniqueBuffer Vulkan::memoryBuffer( vk::BufferCreateInfo& buffer_info ) {

    buffer_info.queueFamilyIndexCount = 1;
    buffer_info.pQueueFamilyIndices = &m_queue_index;
    return m_device->createBufferUnique( buffer_info );
}

/**
 *
 */
vk::UniqueDeviceMemory Vulkan::deviceMemory( vk::Buffer buffer,
        vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags ignored ) {

    vk::MemoryRequirements requirements = m_device->getBufferMemoryRequirements( buffer );
    std::bitset< 32 > memory_bits( requirements.memoryTypeBits );
    vk::DeviceSize memory_size = requirements.size;
    uint32_t memory_id = 0;

    for ( memory_id = 0; memory_id < m_memory_properties.memoryTypeCount; memory_id++ ) {
        if ( memory_bits[ memory_id ] ) {
            auto& memory_type = m_memory_properties.memoryTypes[ memory_id ];
            auto& memory_heap = m_memory_properties.memoryHeaps[ memory_type.heapIndex ];
            if ( ( memory_type.propertyFlags & required ) == required &&
                    ( memory_type.propertyFlags & ignored ) == vk::MemoryPropertyFlags() &&
                    memory_size <= memory_heap.size )
                break;
        }
    }

    if ( memory_id == m_memory_properties.memoryTypeCount )
        throw std::runtime_error( "device memory does not meet the required parameters" );

    vk::MemoryAllocateInfo memory_info;
    memory_info.memoryTypeIndex = memory_id;
    memory_info.allocationSize = memory_size;
    vk::UniqueDeviceMemory memory = m_device->allocateMemoryUnique( memory_info );

    m_device->bindBufferMemory( buffer, *memory, 0 );
    return memory;
}

/**
 *
 */
vk::UniqueDeviceMemory Vulkan::deviceMemory( const vk::ArrayProxy< vk::Buffer >& buffers,
            vk::MemoryPropertyFlags required, vk::MemoryPropertyFlags ignored ) {

    vk::MemoryRequirements requirements[ buffers.size() ];
    std::bitset< 32 > memory_bits;
    vk::DeviceSize align = 0;
    memory_bits.set();

    for ( std::size_t i = 0; i < buffers.size(); i++ ) {
        requirements[ i ] = m_device->getBufferMemoryRequirements( *( buffers.data() + i ) );
        memory_bits &= requirements[ i ].memoryTypeBits;
        align = std::max( align, requirements[ i ].alignment );
    }

    vk::DeviceSize memory_offsets[ buffers.size() ];
    vk::DeviceSize memory_size = 0;
    uint32_t memory_id = 0;

    for ( std::size_t i = 0; i < buffers.size(); i++ ) {
        memory_offsets[ i ] = memory_size;
        memory_size += ( requirements[ i ].size + align - 1 ) / align * align;
    }

    for ( memory_id = 0; memory_id < m_memory_properties.memoryTypeCount; memory_id++ ) {
        if ( memory_bits[ memory_id ] ) {
            auto& memory_type = m_memory_properties.memoryTypes[ memory_id ];
            auto& memory_heap = m_memory_properties.memoryHeaps[ memory_type.heapIndex ];
            if ( ( memory_type.propertyFlags & required ) == required &&
                    ( memory_type.propertyFlags & ignored ) == vk::MemoryPropertyFlags() &&
                    memory_size <= memory_heap.size )
                break;
        }
    }

    if ( memory_id == m_memory_properties.memoryTypeCount )
        throw std::runtime_error( "device memory does not meet the required parameters" );

    vk::MemoryAllocateInfo memory_info;
    memory_info.memoryTypeIndex = memory_id;
    memory_info.allocationSize = memory_size;
    vk::UniqueDeviceMemory memory = m_device->allocateMemoryUnique( memory_info );

    for ( std::size_t i = 0; i < buffers.size(); i++ ) {
        m_device->bindBufferMemory( *( buffers.data() + i ), *memory, memory_offsets[ i ] );
    }

    return memory;
}

/**
 *
 */
vk::UniqueDescriptorSetLayout Vulkan::descriptorSetLayout( vk::DescriptorSetLayoutCreateInfo& descriptor_set_layout_info ) {

    return m_device->createDescriptorSetLayoutUnique( descriptor_set_layout_info );
}

/**
 *
 */
vk::UniqueDescriptorSet Vulkan::descriptorSet( vk::DescriptorSetAllocateInfo& descriptor_set_info ) {

    descriptor_set_info.descriptorPool = *m_descriptor_pool;
    descriptor_set_info.descriptorSetCount = 1;
    return std::move( m_device->allocateDescriptorSetsUnique( descriptor_set_info ).front() );
}

/**
 *
 */
void Vulkan::updateDescriptorSets( const vk::ArrayProxy< const vk::WriteDescriptorSet >& write_descriptor_sets ) {

    m_device->updateDescriptorSets( write_descriptor_sets, {} );
}

/**
 *
 */
vk::UniqueShaderModule Vulkan::shaderModule( vk::ShaderModuleCreateInfo& shader_module_info ) {

    return m_device->createShaderModuleUnique( shader_module_info );
}

/**
 *
 */
vk::UniquePipelineLayout Vulkan::pipelineLayout( vk::PipelineLayoutCreateInfo& pipeline_layout_info ) {

    return m_device->createPipelineLayoutUnique( pipeline_layout_info );
}

/**
 *
 */
vk::UniquePipeline Vulkan::computePipeline( vk::PipelineCache pipeline_cache, vk::ComputePipelineCreateInfo& compute_pipeline_info ) {

#if VK_HEADER_VERSION < 141
    return m_device->createComputePipelineUnique( pipeline_cache, compute_pipeline_info );
#else
    return m_device->createComputePipelineUnique( pipeline_cache, compute_pipeline_info ).value;
#endif
}

/**
 *
 */
void Vulkan::submitPipeline( vk::SubmitInfo& submit_info, uint64_t timeout ) {

    vk::UniqueFence fence = m_device->createFenceUnique( vk::FenceCreateInfo() );
    m_queue.submit( { 1, &submit_info }, *fence );

    vk::Result result = m_device->waitForFences( *fence, VK_TRUE, timeout );
    if ( result != vk::Result::eSuccess )
        throw std::runtime_error( "command buffer execution timed out" );
}

/**
 *
 */
char* Vulkan::mmap( vk::DeviceMemory memory, vk::DeviceSize offset, vk::DeviceSize size ) {

    vk::DeviceSize align = m_device_properties2.properties.limits.nonCoherentAtomSize;
    vk::DeviceSize moffset = offset / align * align;
    vk::DeviceSize mdelta = offset % align;
    vk::DeviceSize msize = size + mdelta;//( size + mdelta + align - 1 ) / align * align;
    return ( char* ) m_device->mapMemory( memory, moffset, msize ) + mdelta;
}

/**
 *
 */
void Vulkan::unmap( vk::DeviceMemory memory ) {

    m_device->unmapMemory( memory );
}

/**
 *
 */
void Vulkan::copy( vk::Buffer buffer_src, vk::Buffer buffer_dst, const vk::BufferCopy& copy ) {

    m_update_buffer->reset( {} );
    m_update_buffer->begin( vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eOneTimeSubmit ) );
    m_update_buffer->copyBuffer( buffer_src, buffer_dst, { 1, &copy } );
    m_update_buffer->end();

    vk::SubmitInfo submit_info;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &*m_update_buffer;
    submitPipeline( submit_info );
}

/**
 *
 */
void Vulkan::createInstance() {

    {
        auto vkGetInstanceProcAddr = m_loader.getProcAddress< PFN_vkGetInstanceProcAddr >( "vkGetInstanceProcAddr" );
        VULKAN_HPP_DEFAULT_DISPATCHER.init( vkGetInstanceProcAddr );
    }

    {
        enum {
            VK_LAYER_KHRONOS_VALIDATION,
            VK_LAYER_LUNARG_STANDARD_VALIDATION,
            VK_LAYERS_COUNT
        };

        enum {
            VK_KHR_SURFACE,
            VK_KHR_XCB_SURFACE,
            VK_EXT_DEBUG_UTILS,
            VK_EXT_COUNT
        };

        std::bitset< VK_LAYERS_COUNT > layers_mask;
        std::bitset< VK_EXT_COUNT > extensions_mask;

        for ( auto& layer : vk::enumerateInstanceLayerProperties() ) {
            if ( false )
                void();
#if !defined( NDEBUG )
            else if ( strcmp( layer.layerName, "VK_LAYER_KHRONOS_validation" ) == 0 )
                layers_mask[ VK_LAYER_KHRONOS_VALIDATION ] = true;
            else if ( strcmp( layer.layerName, "VK_LAYER_LUNARG_standard_validation" ) == 0 )
                layers_mask[ VK_LAYER_LUNARG_STANDARD_VALIDATION ] = true;
#endif
        }

        for ( auto& extension : vk::enumerateInstanceExtensionProperties() ) {
            if ( false )
                void();
#if defined( VK_USE_GRAPHICS_API )
            else if ( strcmp( extension.extensionName, VK_KHR_SURFACE_EXTENSION_NAME ) == 0 )
                extensions_mask[ VK_KHR_SURFACE ] = true;
#endif
#if defined( VK_USE_PLATFORM_XCB_KHR )
            else if ( strcmp( extension.extensionName, VK_KHR_XCB_SURFACE_EXTENSION_NAME ) == 0 )
                extensions_mask[ VK_KHR_XCB_SURFACE ] = true;
#endif
#if !defined( NDEBUG )
            else if ( strcmp( extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME ) == 0 )
                extensions_mask[ VK_EXT_DEBUG_UTILS ] = true;
#endif
        }

        std::vector< const char* > enabled_layers;
        enabled_layers.reserve( layers_mask.count() );
#if !defined( NDEBUG )
        if ( layers_mask[ VK_LAYER_KHRONOS_VALIDATION ] )
            enabled_layers.push_back( "VK_LAYER_KHRONOS_validation" );
        else if ( layers_mask[ VK_LAYER_LUNARG_STANDARD_VALIDATION ] )
            enabled_layers.push_back( "VK_LAYER_LUNARG_standard_validation" );
#endif

        std::vector< const char* > enabled_extensions;
        enabled_extensions.reserve( extensions_mask.count() );
#if defined( VK_USE_GRAPHICS_API )
        if ( extensions_mask[ VK_KHR_SURFACE ] )
            enabled_extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
        else
            throw std::runtime_error( "no surface extension found" );
#endif
#if defined( VK_USE_PLATFORM_XCB_KHR )
        if ( extensions_mask[ VK_KHR_XCB_SURFACE ] )
            enabled_extensions.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
        else
            throw std::runtime_error( "no platform surface extension found" );
#endif
#if !defined( NDEBUG )
        if ( extensions_mask[ VK_EXT_DEBUG_UTILS ] )
            enabled_extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
#endif

        vk::ApplicationInfo application_info;
        application_info.pApplicationName = "resonance_model";
        application_info.apiVersion = VK_API_VERSION_1_2;

        vk::InstanceCreateInfo instance_create_info;
        instance_create_info.pApplicationInfo = &application_info;
        instance_create_info.enabledLayerCount = enabled_layers.size();
        instance_create_info.ppEnabledLayerNames = enabled_layers.data();
        instance_create_info.enabledExtensionCount = enabled_extensions.size();
        instance_create_info.ppEnabledExtensionNames = enabled_extensions.data();
        m_instance = vk::createInstanceUnique( instance_create_info );
        VULKAN_HPP_DEFAULT_DISPATCHER.init( *m_instance );
    }

#if !defined( NDEBUG )
    {
        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_info;
        debug_utils_info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        debug_utils_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
        debug_utils_info.pfnUserCallback = &debugUtilsMessengerCallback;
        m_debug_utils = m_instance->createDebugUtilsMessengerEXTUnique( debug_utils_info );
    }
#endif
}

/**
 *
 */
void Vulkan::createDevice() {

    {
        auto physical_devices = m_instance->enumeratePhysicalDevices();
        vk::PhysicalDevice best_physical_device;
        uint32_t best_device_score = 0;

        vk::PhysicalDeviceProperties2 device_properties2;
        vk::PhysicalDeviceVulkan11Properties device_properties_ext11;
        device_properties2.pNext = &device_properties_ext11;

        enum {
            DEVICE_COMPATIBLE_FLAG = 1 << 0,
            DEVICE_DISCRETE_FLAG = 1 << 1,
            DEVICE_GRAPHICS_FLAG = 1 << 2,
            DEVICE_FLAGS_COUNT
        };

        for ( uint32_t device_index = 0; device_index < physical_devices.size(); device_index++ ) {
            vk::PhysicalDevice physical_device = physical_devices[ device_index ];
            physical_device.getProperties( &device_properties2.properties );

            if ( device_properties2.properties.apiVersion < VK_API_VERSION_1_2 )
                continue;

            physical_device.getProperties2( &device_properties2 );

            if ( !( device_properties_ext11.subgroupSupportedStages & vk::ShaderStageFlagBits::eCompute ) ||
                    !( device_properties_ext11.subgroupSupportedOperations & vk::SubgroupFeatureFlagBits::eBasic ) ||
                    !( device_properties_ext11.subgroupSupportedOperations & vk::SubgroupFeatureFlagBits::eVote ) ||
                    !( device_properties_ext11.subgroupSupportedOperations & vk::SubgroupFeatureFlagBits::eBallot ) ||
                    !( device_properties_ext11.subgroupSupportedOperations & vk::SubgroupFeatureFlagBits::eArithmetic ) )
                continue;

            uint32_t device_score = DEVICE_COMPATIBLE_FLAG;
            if ( device_properties2.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu )
                device_score |= DEVICE_DISCRETE_FLAG;

            uint32_t queue_index = -1;
            std::vector< vk::QueueFamilyProperties > queue_properties =
                    physical_device.getQueueFamilyProperties();

            for ( uint32_t index = 0; index < queue_properties.size(); index++ ) {
                if ( !( queue_properties[ index ].queueFlags & vk::QueueFlagBits::eCompute ) ||
                        !( queue_properties[ index ].queueFlags & vk::QueueFlagBits::eTransfer ) )
                    continue;
#if defined( VK_USE_GRAPHICS_API )
                if ( queue_properties[ index ].queueFlags & vk::QueueFlagBits::eGraphics )
                    device_score |= DEVICE_GRAPHICS_FLAG;
                else
                    continue;
#endif
                queue_index = index;
                break;
            }

            if ( queue_index == uint32_t( -1 ) )
                continue;

            if ( best_device_score < device_score ) {
                best_physical_device = physical_device;
                best_device_score = device_score;
                m_device_index = device_index;
                m_queue_index = queue_index;
                m_device_properties2 = device_properties2;
                m_device_properties_ext11 = device_properties_ext11;
            }
        }

        if ( best_physical_device ) {
            m_physical_device = best_physical_device;
            m_memory_properties = best_physical_device.getMemoryProperties();
        }
        else
            throw std::runtime_error( "no suitable device found" );
    }

    {
        enum {
            VK_EXT_SHADER_ATOMIC_FLOAT,
            VK_KHR_SWAPCHAIN,
            VK_KHR_SHADER_NON_SEMANTIC_INFO,
            VK_EXT_COUNT
        };

        std::bitset< VK_EXT_COUNT > extensions_mask;

        for ( auto& extension : m_physical_device.enumerateDeviceExtensionProperties() ) {
            if ( false )
                void();
            else if ( strcmp( extension.extensionName, VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME ) == 0 )
                extensions_mask[ VK_EXT_SHADER_ATOMIC_FLOAT ] = true;
#if defined( VK_USE_GRAPHICS_API )
            else if ( strcmp( extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME ) == 0 )
                extensions_mask[ VK_KHR_SWAPCHAIN ] = true;
#endif
#if !defined( NDEBUG )
            else if ( strcmp( extension.extensionName, VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME ) == 0 )
                extensions_mask[ VK_KHR_SHADER_NON_SEMANTIC_INFO ] = true;
#endif
        }

        std::vector< const char* > enabled_extensions;
        enabled_extensions.reserve( extensions_mask.count() );
        if ( extensions_mask[ VK_EXT_SHADER_ATOMIC_FLOAT ] )
            enabled_extensions.push_back( VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME );
#if defined( VK_USE_GRAPHICS_API )
        if ( extensions_mask[ VK_KHR_SWAPCHAIN ] )
            enabled_extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
        else
            throw std::runtime_error( "no swapchain extension found" );
#endif
#if !defined( NDEBUG )
        if ( extensions_mask[ VK_KHR_SHADER_NON_SEMANTIC_INFO ] )
            enabled_extensions.push_back( VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME );
#endif

        m_device_features2.pNext = &m_device_features_ext11;
        m_device_features_ext11.pNext = &m_device_features_ext12;
        if ( extensions_mask[ VK_EXT_SHADER_ATOMIC_FLOAT ] )
            m_device_features_ext12.pNext = &m_device_features_atomic_float;
        m_physical_device.getFeatures2( &m_device_features2 );

        if ( m_device_features2.features.shaderInt64 == VK_FALSE )
            throw std::runtime_error( "no int64 support" );

        if ( m_device_features2.features.shaderFloat64 == VK_FALSE )
            throw std::runtime_error( "no float64 support" );

        if ( m_device_features_ext11.storageBuffer16BitAccess == VK_FALSE )
            throw std::runtime_error( "no int16 storage buffer support" );

        if ( m_device_features_ext12.storageBuffer8BitAccess == VK_FALSE )
            throw std::runtime_error( "no int8 storage buffer support" );

        if ( m_device_features_ext12.shaderBufferInt64Atomics == VK_FALSE &&
                m_device_features_atomic_float.shaderBufferFloat64AtomicAdd == VK_FALSE ) {
            throw std::runtime_error( "no buffer atomic add support" );
        }

        float queue_priority = 1.0f;

        vk::DeviceQueueCreateInfo queue_info;
        queue_info.queueFamilyIndex = m_queue_index;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;

        vk::DeviceCreateInfo device_info;
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        device_info.enabledExtensionCount = enabled_extensions.size();
        device_info.ppEnabledExtensionNames = enabled_extensions.data();
        device_info.pNext = &m_device_features2;
        m_device = m_physical_device.createDeviceUnique( device_info );
        VULKAN_HPP_DEFAULT_DISPATCHER.init( *m_device );
    }

    {
        std::array< vk::DescriptorPoolSize, 2 > descriptor_pool_sizes {
            vk::DescriptorPoolSize( vk::DescriptorType::eUniformBuffer, 8 ),
            vk::DescriptorPoolSize( vk::DescriptorType::eStorageBuffer, 64 ) };

        vk::DescriptorPoolCreateInfo descriptor_pool_info;
        descriptor_pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        descriptor_pool_info.maxSets = 8;
        descriptor_pool_info.poolSizeCount = descriptor_pool_sizes.size();
        descriptor_pool_info.pPoolSizes = descriptor_pool_sizes.data();
        m_descriptor_pool = m_device->createDescriptorPoolUnique( descriptor_pool_info );
    }

    {
        vk::CommandPoolCreateInfo command_pool_info;
        command_pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        command_pool_info.queueFamilyIndex = m_queue_index;
        m_command_pool = m_device->createCommandPoolUnique( command_pool_info );
    }

    {
        vk::CommandBufferAllocateInfo command_buffer_info;
        command_buffer_info.commandPool = *m_command_pool;
        command_buffer_info.commandBufferCount = 1;
        m_update_buffer = std::move( m_device->allocateCommandBuffersUnique( command_buffer_info ).front() );
    }

    {
        m_queue = m_device->getQueue( m_queue_index, 0 );
    }

}

/**
 *
 */
VkBool32 Vulkan::debugUtilsMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_types,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* /*user_data*/ ) {

    std::cerr << "--- " <<
            vk::to_string( vk::DebugUtilsMessageSeverityFlagBitsEXT( message_severity ) ) <<
            vk::to_string( vk::DebugUtilsMessageTypeFlagsEXT( message_types ) ) << " ---\n";

    std::cerr << "ID: " << callback_data->pMessageIdName << "\nMESSAGE: " << callback_data->pMessage;
    if ( std::string_view( callback_data->pMessage ).back() != '\n' )
        std::cerr << "\n";

    if ( callback_data->queueLabelCount > 0 ) {
        std::cerr << "QUEUES: ";
        for ( uint8_t i = 0; i < callback_data->queueLabelCount; i++ )
            std::cerr << callback_data->pQueueLabels[ i ].pLabelName << " ";
        std::cerr << "\n";
    }

    if ( callback_data->cmdBufLabelCount > 0 ) {
        std::cerr << "COMMANDS: ";
        for ( uint8_t i = 0; i < callback_data->cmdBufLabelCount; i++ )
            std::cerr << callback_data->pCmdBufLabels[ i ].pLabelName << " ";
        std::cerr << "\n";
    }

    if ( callback_data->objectCount > 0 ) {
        std::cerr << "OBJECTS: ";
        for ( uint8_t i = 0; i < callback_data->objectCount; i++ ) {
            auto object = callback_data->pObjects[ i ];
            std::cerr << vk::to_string( vk::ObjectType( object.objectType ) );
            if ( object.pObjectName )
                std::cerr << "{ " << object.pObjectName << " } ";
            else
                std::cerr << "{} ";
        }
        std::cerr << "\n";
    }

    return VK_TRUE;
}
