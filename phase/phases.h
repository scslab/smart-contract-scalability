#pragma once

/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

namespace scs
{

class GlobalContext;
struct GroundhogBlockContext;
class SisyphusGlobalContext;
struct SisyphusBlockContext;


void phase_finish_block(GlobalContext& global_structures, GroundhogBlockContext& block_structures);
void phase_undo_block(GlobalContext& global_structures, GroundhogBlockContext& block_structures);

void phase_finish_block(SisyphusGlobalContext& global_structures, SisyphusBlockContext& block_structures);
void phase_undo_block(SisyphusGlobalContext& global_structures, SisyphusBlockContext& block_structures);

} /* scs */
