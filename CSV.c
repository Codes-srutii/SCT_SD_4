#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

// Structure to hold the response data from the HTTP request
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback function to write data received from cURL into a memory buffer
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Function to perform an HTTP GET request and retrieve the HTML content
char *fetch_html(const char *url) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1); // Will be grown by realloc in the callback
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        return NULL;
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return chunk.memory;
}

// Function to parse HTML and extract product information
void parse_html(const char *html) {
    htmlDocPtr doc = htmlReadMemory(html, strlen(html), NULL, NULL, HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse HTML\n");
        return;
    }

    // Create an XPath context
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL) {
        fprintf(stderr, "Failed to create XPath context\n");
        xmlFreeDoc(doc);
        return;
    }

    // XPath expressions to extract product details
    const char *name_xpath = "//div[@class='product-name']";  // Adjust based on actual site
    const char *price_xpath = "//span[@class='product-price']";
    const char *rating_xpath = "//div[@class='product-rating']";

    // Extract product names
    xmlXPathObjectPtr nameObj = xmlXPathEvalExpression((xmlChar *)name_xpath, xpathCtx);
    if (nameObj != NULL && xmlXPathNodeSetIsEmpty(nameObj->nodesetval) == 0) {
        for (int i = 0; i < nameObj->nodesetval->nodeNr; i++) {
            xmlNode *node = nameObj->nodesetval->nodeTab[i];
            printf("Product Name: %s\n", xmlNodeGetContent(node));
        }
    }

    // Similarly extract prices and ratings (not fully implemented here)

    xmlXPathFreeObject(nameObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
}

// Function to save extracted data to CSV (dummy example, customize as needed)
void save_to_csv(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }

    // Write headers
    fprintf(file, "Product Name,Price,Rating\n");

    // Write some dummy data (replace with actual scraped data)
    fprintf(file, "Sample Product,19.99,4.5\n");

    fclose(file);
}

int main(void) {
    const char *url = "https://www.example.com/products"; // Replace with actual URL

    // Step 1: Fetch the HTML content
    char *html_content = fetch_html(url);
    if (html_content == NULL) {
        fprintf(stderr, "Failed to fetch HTML content\n");
        return 1;
    }

    // Step 2: Parse the HTML to extract product info
    parse_html(html_content);

    // Step 3: Save the extracted data to a CSV file
    save_to_csv("products.csv");

    free(html_content);
    return 0;
}