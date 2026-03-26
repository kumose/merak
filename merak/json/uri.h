// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.

#ifndef MERAK_JSON_URI_H_
#define MERAK_JSON_URI_H_

#include <merak/json/internal/strfunc.h>
#if defined(__clang__)
MERAK_JSON_DIAG_PUSH
MERAK_JSON_DIAG_OFF(c++98-compat)
#elif defined(_MSC_VER)
MERAK_JSON_DIAG_OFF(4512) // assignment operator could not be generated
#endif

namespace merak::json {

///////////////////////////////////////////////////////////////////////////////
// GenericUri

template <typename ValueType, typename Allocator=CrtAllocator>
class GenericUri {
public:
    typedef typename ValueType::char_type char_type;

    typedef std::basic_string<char_type> String;

    //! Constructors
    GenericUri(Allocator* allocator = 0) : uri_(), base_(), scheme_(), auth_(), path_(), query_(), frag_(), allocator_(allocator), ownAllocator_() {
    }

    GenericUri(const char_type* uri, SizeType len, Allocator* allocator = 0) : uri_(), base_(), scheme_(), auth_(), path_(), query_(), frag_(), allocator_(allocator), ownAllocator_() {
        parse(uri, len);
    }

    GenericUri(const char_type* uri, Allocator* allocator = 0) : uri_(), base_(), scheme_(), auth_(), path_(), query_(), frag_(), allocator_(allocator), ownAllocator_() {
        parse(uri, internal::StrLen<char_type>(uri));
    }

    // Use with specializations of GenericValue
    template<typename T> GenericUri(const T& uri, Allocator* allocator = 0) : uri_(), base_(), scheme_(), auth_(), path_(), query_(), frag_(), allocator_(allocator), ownAllocator_() {
        const char_type* u = uri.template get<const char_type*>(); // TypeHelper from document.h
        parse(u, internal::StrLen<char_type>(u));
    }


    GenericUri(const String& uri, Allocator* allocator = 0) : uri_(), base_(), scheme_(), auth_(), path_(), query_(), frag_(), allocator_(allocator), ownAllocator_() {
        parse(uri.c_str(), internal::StrLen<char_type>(uri.c_str()));
    }


    //! Copy constructor
    GenericUri(const GenericUri& rhs) : uri_(), base_(), scheme_(), auth_(), path_(), query_(), frag_(), allocator_(), ownAllocator_() {
        *this = rhs;
    }

    //! Copy constructor
    GenericUri(const GenericUri& rhs, Allocator* allocator) : uri_(), base_(), scheme_(), auth_(), path_(), query_(), frag_(), allocator_(allocator), ownAllocator_() {
        *this = rhs;
    }

    //! Destructor.
    ~GenericUri() {
        Free();
        MERAK_JSON_DELETE(ownAllocator_);
    }

    //! Assignment operator
    GenericUri& operator=(const GenericUri& rhs) {
        if (this != &rhs) {
            // Do not delete ownAllocator
            Free();
            Allocate(rhs.get_string_length());
            auth_ = CopyPart(scheme_, rhs.scheme_, rhs.GetSchemeStringLength());
            path_ = CopyPart(auth_, rhs.auth_, rhs.GetAuthStringLength());
            query_ = CopyPart(path_, rhs.path_, rhs.GetPathStringLength());
            frag_ = CopyPart(query_, rhs.query_, rhs.GetQueryStringLength());
            base_ = CopyPart(frag_, rhs.frag_, rhs.GetFragStringLength());
            uri_ = CopyPart(base_, rhs.base_, rhs.GetBaseStringLength());
            CopyPart(uri_, rhs.uri_, rhs.get_string_length());
        }
        return *this;
    }

    //! Getters
    // Use with specializations of GenericValue
    template<typename T> void get(T& uri, Allocator& allocator) {
        uri.template set<const char_type*>(this->get_string(), allocator); // TypeHelper from document.h
    }

    const char_type* get_string() const { return uri_; }
    SizeType get_string_length() const { return uri_ == 0 ? 0 : internal::StrLen<char_type>(uri_); }
    const char_type* GetBaseString() const { return base_; }
    SizeType GetBaseStringLength() const { return base_ == 0 ? 0 : internal::StrLen<char_type>(base_); }
    const char_type* GetSchemeString() const { return scheme_; }
    SizeType GetSchemeStringLength() const { return scheme_ == 0 ? 0 : internal::StrLen<char_type>(scheme_); }
    const char_type* GetAuthString() const { return auth_; }
    SizeType GetAuthStringLength() const { return auth_ == 0 ? 0 : internal::StrLen<char_type>(auth_); }
    const char_type* GetPathString() const { return path_; }
    SizeType GetPathStringLength() const { return path_ == 0 ? 0 : internal::StrLen<char_type>(path_); }
    const char_type* GetQueryString() const { return query_; }
    SizeType GetQueryStringLength() const { return query_ == 0 ? 0 : internal::StrLen<char_type>(query_); }
    const char_type* GetFragString() const { return frag_; }
    SizeType GetFragStringLength() const { return frag_ == 0 ? 0 : internal::StrLen<char_type>(frag_); }


    static String get(const GenericUri& uri) { return String(uri.get_string(), uri.get_string_length()); }
    static String GetBase(const GenericUri& uri) { return String(uri.GetBaseString(), uri.GetBaseStringLength()); }
    static String GetScheme(const GenericUri& uri) { return String(uri.GetSchemeString(), uri.GetSchemeStringLength()); }
    static String GetAuth(const GenericUri& uri) { return String(uri.GetAuthString(), uri.GetAuthStringLength()); }
    static String GetPath(const GenericUri& uri) { return String(uri.GetPathString(), uri.GetPathStringLength()); }
    static String GetQuery(const GenericUri& uri) { return String(uri.GetQueryString(), uri.GetQueryStringLength()); }
    static String GetFrag(const GenericUri& uri) { return String(uri.GetFragString(), uri.GetFragStringLength()); }


    //! Equality operators
    bool operator==(const GenericUri& rhs) const {
        return Match(rhs, true);
    }

    bool operator!=(const GenericUri& rhs) const {
        return !Match(rhs, true);
    }

    bool Match(const GenericUri& uri, bool full = true) const {
        char_type* s1;
        char_type* s2;
        if (full) {
            s1 = uri_;
            s2 = uri.uri_;
        } else {
            s1 = base_;
            s2 = uri.base_;
        }
        if (s1 == s2) return true;
        if (s1 == 0 || s2 == 0) return false;
        return internal::StrCmp<char_type>(s1, s2) == 0;
    }

    //! Resolve this URI against another (base) URI in accordance with URI resolution rules.
    // See https://tools.ietf.org/html/rfc3986
    // Use for resolving an id or $ref with an in-scope id.
    // Returns a new GenericUri for the resolved URI.
    GenericUri Resolve(const GenericUri& baseuri, Allocator* allocator = 0) {
        GenericUri resuri;
        resuri.allocator_ = allocator;
        // Ensure enough space for combining paths
        resuri.Allocate(get_string_length() + baseuri.get_string_length() + 1); // + 1 for joining slash

        if (!(GetSchemeStringLength() == 0)) {
            // Use all of this URI
            resuri.auth_ = CopyPart(resuri.scheme_, scheme_, GetSchemeStringLength());
            resuri.path_ = CopyPart(resuri.auth_, auth_, GetAuthStringLength());
            resuri.query_ = CopyPart(resuri.path_, path_, GetPathStringLength());
            resuri.frag_ = CopyPart(resuri.query_, query_, GetQueryStringLength());
            resuri.RemoveDotSegments();
        } else {
            // Use the base scheme
            resuri.auth_ = CopyPart(resuri.scheme_, baseuri.scheme_, baseuri.GetSchemeStringLength());
            if (!(GetAuthStringLength() == 0)) {
                // Use this auth, path, query
                resuri.path_ = CopyPart(resuri.auth_, auth_, GetAuthStringLength());
                resuri.query_ = CopyPart(resuri.path_, path_, GetPathStringLength());
                resuri.frag_ = CopyPart(resuri.query_, query_, GetQueryStringLength());
                resuri.RemoveDotSegments();
            } else {
                // Use the base auth
                resuri.path_ = CopyPart(resuri.auth_, baseuri.auth_, baseuri.GetAuthStringLength());
                if (GetPathStringLength() == 0) {
                    // Use the base path
                    resuri.query_ = CopyPart(resuri.path_, baseuri.path_, baseuri.GetPathStringLength());
                    if (GetQueryStringLength() == 0) {
                        // Use the base query
                        resuri.frag_ = CopyPart(resuri.query_, baseuri.query_, baseuri.GetQueryStringLength());
                    } else {
                        // Use this query
                        resuri.frag_ = CopyPart(resuri.query_, query_, GetQueryStringLength());
                    }
                } else {
                    if (path_[0] == '/') {
                        // Absolute path - use all of this path
                        resuri.query_ = CopyPart(resuri.path_, path_, GetPathStringLength());
                        resuri.RemoveDotSegments();
                    } else {
                        // Relative path - append this path to base path after base path's last slash
                        size_t pos = 0;
                        if (!(baseuri.GetAuthStringLength() == 0) && baseuri.GetPathStringLength() == 0) {
                            resuri.path_[pos] = '/';
                            pos++;
                        }
                        size_t lastslashpos = baseuri.GetPathStringLength();
                        while (lastslashpos > 0) {
                            if (baseuri.path_[lastslashpos - 1] == '/') break;
                            lastslashpos--;
                        }
                        std::memcpy(&resuri.path_[pos], baseuri.path_, lastslashpos * sizeof(char_type));
                        pos += lastslashpos;
                        resuri.query_ = CopyPart(&resuri.path_[pos], path_, GetPathStringLength());
                        resuri.RemoveDotSegments();
                    }
                    // Use this query
                    resuri.frag_ = CopyPart(resuri.query_, query_, GetQueryStringLength());
                }
            }
        }
        // Always use this frag
        resuri.base_ = CopyPart(resuri.frag_, frag_, GetFragStringLength());

        // Re-constitute base_ and uri_
        resuri.SetBase();
        resuri.uri_ = resuri.base_ + resuri.GetBaseStringLength() + 1;
        resuri.SetUri();
        return resuri;
    }

    //! Get the allocator of this GenericUri.
    Allocator& get_allocator() { return *allocator_; }

private:
    // Allocate memory for a URI
    // Returns total amount allocated
    std::size_t Allocate(std::size_t len) {
        // Create own allocator if user did not supply.
        if (!allocator_)
            ownAllocator_ =  allocator_ = MERAK_JSON_NEW(Allocator)();

        // Allocate one block containing each part of the URI (5) plus base plus full URI, all null terminated.
        // Order: scheme, auth, path, query, frag, base, uri
        // Note need to set, increment, assign in 3 stages to avoid compiler warning bug.
        size_t total = (3 * len + 7) * sizeof(char_type);
        scheme_ = static_cast<char_type*>(allocator_->Malloc(total));
        *scheme_ = '\0';
        auth_ = scheme_;
        auth_++;
        *auth_ = '\0';
        path_ = auth_;
        path_++;
        *path_ = '\0';
        query_ = path_;
        query_++;
        *query_ = '\0';
        frag_ = query_;
        frag_++;
        *frag_ = '\0';
        base_ = frag_;
        base_++;
        *base_ = '\0';
        uri_ = base_;
        uri_++;
        *uri_ = '\0';
        return total;
    }

    // Free memory for a URI
    void Free() {
        if (scheme_) {
            Allocator::Free(scheme_);
            scheme_ = 0;
        }
    }

    // parse a URI into constituent scheme, authority, path, query, & fragment parts
    // Supports URIs that match regex ^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))? as per
    // https://tools.ietf.org/html/rfc3986
    void parse(const char_type* uri, std::size_t len) {
        std::size_t start = 0, pos1 = 0, pos2 = 0;
        Allocate(len);

        // Look for scheme ([^:/?#]+):)?
        if (start < len) {
            while (pos1 < len) {
                if (uri[pos1] == ':') break;
                pos1++;
            }
            if (pos1 != len) {
                while (pos2 < len) {
                    if (uri[pos2] == '/') break;
                    if (uri[pos2] == '?') break;
                    if (uri[pos2] == '#') break;
                    pos2++;
                }
                if (pos1 < pos2) {
                    pos1++;
                    std::memcpy(scheme_, &uri[start], pos1 * sizeof(char_type));
                    scheme_[pos1] = '\0';
                    start = pos1;
                }
            }
        }
        // Look for auth (//([^/?#]*))?
        // Note need to set, increment, assign in 3 stages to avoid compiler warning bug.
        auth_ = scheme_ + GetSchemeStringLength();
        auth_++;
        *auth_ = '\0';
        if (start < len - 1 && uri[start] == '/' && uri[start + 1] == '/') {
            pos2 = start + 2;
            while (pos2 < len) {
                if (uri[pos2] == '/') break;
                if (uri[pos2] == '?') break;
                if (uri[pos2] == '#') break;
                pos2++;
            }
            std::memcpy(auth_, &uri[start], (pos2 - start) * sizeof(char_type));
            auth_[pos2 - start] = '\0';
            start = pos2;
        }
        // Look for path ([^?#]*)
        // Note need to set, increment, assign in 3 stages to avoid compiler warning bug.
        path_ = auth_ + GetAuthStringLength();
        path_++;
        *path_ = '\0';
        if (start < len) {
            pos2 = start;
            while (pos2 < len) {
                if (uri[pos2] == '?') break;
                if (uri[pos2] == '#') break;
                pos2++;
            }
            if (start != pos2) {
                std::memcpy(path_, &uri[start], (pos2 - start) * sizeof(char_type));
                path_[pos2 - start] = '\0';
                if (path_[0] == '/')
                    RemoveDotSegments();   // absolute path - normalize
                start = pos2;
            }
        }
        // Look for query (\?([^#]*))?
        // Note need to set, increment, assign in 3 stages to avoid compiler warning bug.
        query_ = path_ + GetPathStringLength();
        query_++;
        *query_ = '\0';
        if (start < len && uri[start] == '?') {
            pos2 = start + 1;
            while (pos2 < len) {
                if (uri[pos2] == '#') break;
                pos2++;
            }
            if (start != pos2) {
                std::memcpy(query_, &uri[start], (pos2 - start) * sizeof(char_type));
                query_[pos2 - start] = '\0';
                start = pos2;
            }
        }
        // Look for fragment (#(.*))?
        // Note need to set, increment, assign in 3 stages to avoid compiler warning bug.
        frag_ = query_ + GetQueryStringLength();
        frag_++;
        *frag_ = '\0';
        if (start < len && uri[start] == '#') {
            std::memcpy(frag_, &uri[start], (len - start) * sizeof(char_type));
            frag_[len - start] = '\0';
        }

        // Re-constitute base_ and uri_
        base_ = frag_ + GetFragStringLength() + 1;
        SetBase();
        uri_ = base_ + GetBaseStringLength() + 1;
        SetUri();
    }

    // Reconstitute base
    void SetBase() {
        char_type* next = base_;
        std::memcpy(next, scheme_, GetSchemeStringLength() * sizeof(char_type));
        next+= GetSchemeStringLength();
        std::memcpy(next, auth_, GetAuthStringLength() * sizeof(char_type));
        next+= GetAuthStringLength();
        std::memcpy(next, path_, GetPathStringLength() * sizeof(char_type));
        next+= GetPathStringLength();
        std::memcpy(next, query_, GetQueryStringLength() * sizeof(char_type));
        next+= GetQueryStringLength();
        *next = '\0';
    }

    // Reconstitute uri
    void SetUri() {
        char_type* next = uri_;
        std::memcpy(next, base_, GetBaseStringLength() * sizeof(char_type));
        next+= GetBaseStringLength();
        std::memcpy(next, frag_, GetFragStringLength() * sizeof(char_type));
        next+= GetFragStringLength();
        *next = '\0';
    }

    // Copy a part from one GenericUri to another
    // Return the pointer to the next part to be copied to
    char_type* CopyPart(char_type* to, char_type* from, std::size_t len) {
        MERAK_JSON_ASSERT(to != 0);
        MERAK_JSON_ASSERT(from != 0);
        std::memcpy(to, from, len * sizeof(char_type));
        to[len] = '\0';
        char_type* next = to + len + 1;
        return next;
    }

    // Remove . and .. segments from the path_ member.
    // https://tools.ietf.org/html/rfc3986
    // This is done in place as we are only removing segments.
    void RemoveDotSegments() {
        std::size_t pathlen = GetPathStringLength();
        std::size_t pathpos = 0;  // Position in path_
        std::size_t newpos = 0;   // Position in new path_

        // Loop through each segment in original path_
        while (pathpos < pathlen) {
            // Get next segment, bounded by '/' or end
            size_t slashpos = 0;
            while ((pathpos + slashpos) < pathlen) {
                if (path_[pathpos + slashpos] == '/') break;
                slashpos++;
            }
            // Check for .. and . segments
            if (slashpos == 2 && path_[pathpos] == '.' && path_[pathpos + 1] == '.') {
                // Backup a .. segment in the new path_
                // We expect to find a previously added slash at the end or nothing
                MERAK_JSON_ASSERT(newpos == 0 || path_[newpos - 1] == '/');
                size_t lastslashpos = newpos;
                // Make sure we don't go beyond the start segment
                if (lastslashpos > 1) {
                    // Find the next to last slash and back up to it
                    lastslashpos--;
                    while (lastslashpos > 0) {
                        if (path_[lastslashpos - 1] == '/') break;
                        lastslashpos--;
                    }
                    // Set the new path_ position
                    newpos = lastslashpos;
                }
            } else if (slashpos == 1 && path_[pathpos] == '.') {
                // Discard . segment, leaves new path_ unchanged
            } else {
                // Move any other kind of segment to the new path_
                MERAK_JSON_ASSERT(newpos <= pathpos);
                std::memmove(&path_[newpos], &path_[pathpos], slashpos * sizeof(char_type));
                newpos += slashpos;
                // Add slash if not at end
                if ((pathpos + slashpos) < pathlen) {
                    path_[newpos] = '/';
                    newpos++;
                }
            }
            // Move to next segment
            pathpos += slashpos + 1;
        }
        path_[newpos] = '\0';
    }

    char_type* uri_;    // Everything
    char_type* base_;   // Everything except fragment
    char_type* scheme_; // Includes the :
    char_type* auth_;   // Includes the //
    char_type* path_;   // Absolute if starts with /
    char_type* query_;  // Includes the ?
    char_type* frag_;   // Includes the #

    Allocator* allocator_;      //!< The current allocator. It is either user-supplied or equal to ownAllocator_.
    Allocator* ownAllocator_;   //!< Allocator owned by this Uri.
};

//! GenericUri for Value (UTF-8, default allocator).
typedef GenericUri<Value> Uri;

}  // namespace merak::json

#if defined(__clang__)
MERAK_JSON_DIAG_POP
#endif

#endif // MERAK_JSON_URI_H_
